/*
 * The code is a part of our project called VEGITO, which retrofits
 * high availability mechanism to tame hybrid transaction/analytical
 * processing.
 *
 * Copyright (c) 2021 Shanghai Jiao Tong University.
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://ipads.se.sjtu.edu.cn/projects/vegito
 *
 */

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include "graph/graph_store.h"
#include "property/property_col_paged.h"
#include "util/bitset.h"

#define LAZY_PAGE_ALLOC 1

using std::vector;

namespace {
constexpr uint32_t MAX_CACHED_TABLES = 128;
constexpr uint32_t MAX_CACHED_COLS = 128;

// table id (MAX_CACHED_TABLES)
thread_local uint64_t cached_ver[MAX_CACHED_TABLES];
// table id (MAX_CACHED_TABLES), column id (MAX_CACHED_COLS)
thread_local uint32_t cached_page_num[MAX_CACHED_TABLES][MAX_CACHED_COLS];
thread_local void* cached_page[MAX_CACHED_TABLES][MAX_CACHED_COLS] = {
    {nullptr}};

}  // namespace

namespace gart {
namespace property {

PropertyColPaged::PropertyColPaged(Property::Schema s, uint64_t max_items,
                                   memory::BufferManager& buf_mgr,
                                   const vector<uint32_t>* split)
    : Property(max_items, buf_mgr),
      table_id_(s.table_id),
      cols_(s.cols),
      col_v6d_oids_(s.cols.size()),
      col_v6d_offsets_(s.cols.size()),
      flex_bufs_(s.cols.size()),
      fixCols_(s.cols.size(), nullptr),
      flexCols_(s.cols.size()) {
  // each column
  val_len_ = 0;
  for (int i = 0; i < cols_.size(); i++) {
    size_t vlen = cols_[i].vlen;
    val_off_.push_back(val_len_);
    val_lens_.push_back(vlen);
    val_type_.push_back(cols_[i].vtype);

    val_len_ += vlen;
    if (cols_[i].page_size > max_items_)
      cols_[i].page_size = max_items_;
    if (cols_[i].page_size == 0)
      cols_[i].page_size = 4 * 1024;
  }

  pcols_.reserve(cols_.size());
  if (split == nullptr) {
    for (int i = 0; i < cols_.size(); ++i) {
      pcols_.emplace_back(i, 0);
      split_.emplace_back(i);
      split_vlen_.emplace_back(val_lens_[i]);
    }
  } else {
    assert(split->size() > 0);
    for (int s = 0; s < split->size(); ++s) {
      int begin = (*split)[s];
      int end = (s == split->size() - 1) ? cols_.size() : (*split)[s + 1];
      uint64_t off = 0;
      for (int i = begin; i < end; ++i) {
        pcols_.emplace_back(s, off);
        off += val_lens_[i];
      }
      split_.emplace_back(s);
      split_vlen_.emplace_back(off);
    }
  }
  assert(pcols_.size() == cols_.size());

  for (int i = 0; i < cols_.size(); ++i) {
    size_t vlen = val_lens_[i];
    // divided into 2 types according to "updatable"
    cols_[i].updatable = true;
    if (cols_[i].updatable) {
      size_t page_sz = cols_[i].page_size;
      int page_num = (max_items_ + page_sz - 1) / page_sz;
      size_t null_bitmap_size = BYTE_SIZE(page_sz * cols_[i].real_column_num);
      assert(page_num != 0);
      flexCols_[i].locks.assign(page_num, 0);
      flexCols_[i].pages.assign(page_num, nullptr);
      flexCols_[i].old_pages.assign(page_num, nullptr);
      // size_t total_sz =
      //     FlexColHeader::size(page_num) +
      //     page_num * (sizeof(Page) + page_sz * vlen + null_bitmap_size);
      // total_sz *= 1.5;

      size_t total_sz = FlexColHeader::size(page_num);
      char* buf = v6d_malloc(total_sz, col_v6d_oids_[i], col_v6d_offsets_[i]);
      FlexColHeader* header = reinterpret_cast<FlexColHeader*>(buf);
      flex_bufs_[i].total_sz = total_sz;
      flex_bufs_[i].buf = buf;
      flex_bufs_[i].header = header;
      flex_bufs_[i].allocated_sz = 0 + FlexColHeader::size(page_num);

      header->num_row_per_page = page_sz;

      //       for (int p = 0; p < page_num; ++p) {
      // #if LAZY_PAGE_ALLOC == 0
      //         Page* page = getNewPage_(page_sz, vlen,
      //         cols_[i].real_column_num, -1,
      //                                  nullptr, i, p);
      //         flexCols_[i].old_pages[p] = page;
      // #else
      //         Page* page =
      //             getInitPage_(page_sz, vlen, cols_[i].real_column_num, i,
      //             p);
      // #endif
      //         flexCols_[i].pages[p] = page;
      //       }
    } else {
      fixCols_[i] =
          v6d_malloc(vlen * max_items_, col_v6d_oids_[i], col_v6d_offsets_[i]);
      printf("Vlabel %d column %d (fixed), malloc %lf GB\n", table_id_, i,
             vlen * max_items_ / 1024.0 / 1024 / 1024);
    }
  }

  blob_metas_.resize(cols_.size());
  for (int i = 0; i < cols_.size(); ++i) {
    gart::VPropMeta& meta = blob_metas_[i];
    meta.init(i, val_lens_[i], cols_[i].updatable, cols_[i].vtype);

    meta.set_oid(col_v6d_oids_[i], col_v6d_offsets_[i]);
  }
}

PropertyColPaged::~PropertyColPaged() {
  for (int i = 0; i < cols_.size(); i++) {
    size_t page_sz = cols_[i].page_size;
    if (cols_[i].updatable) {
      int page_num = (max_items_ + page_sz - 1) / page_sz;
      for (int p = 0; p < page_num; ++p)
        free(flexCols_[i].pages[p]);
    } else {
      delete[] fixCols_[i];
    }
  }
}

// NOTE: page_sz is number of objects, instead of bytes
inline PropertyColPaged::Page* PropertyColPaged::getNewPage_(
    uint64_t page_sz, uint64_t vlen, uint64_t real_column_num, uint64_t ver,
    Page* prev, uint64_t prop_id, uint64_t pg_num) {
  FlexBuf& flex_buf = flex_bufs_[prop_id];
  char* buf = nullptr;
  uint32_t pg_sz =
      sizeof(Page) + vlen * page_sz + BYTE_SIZE(page_sz * real_column_num);

  uint64_t cur_ptr;
  buf = v6d_malloc(pg_sz, col_v6d_oids_[prop_id], cur_ptr);
  assert(buf);
  Page* ret = new (buf) Page(ver, cur_ptr, prev);

  if (prev != nullptr) {
    ret->prev_ptr = prev->v6d_offset;
  }

  if (page_sz != 1 && prev != nullptr) {
    memcpy(ret->content, prev->content,
           page_sz * vlen + BYTE_SIZE(page_sz * real_column_num));
#if UPDATE_STAT
    stat_.num_copy += BYTE_SIZE(page_sz * real_column_num);  // bitmap size
    stat_.num_copy += vlen * page_sz;                        // size
    // ++stat_.num_copy;                                        // count
#endif
  }
  flex_buf.header->page_ptr[pg_num] = cur_ptr;
  return ret;
}

inline PropertyColPaged::Page* PropertyColPaged::findWithInsertPage_(
    int colID, uint64_t pg_num, uint64_t version) {
  const Property::Column& col = cols_[colID];

  FlexCol& flex = flexCols_[colID];
  assert(pg_num < flex.pages.size());
  Page* page = flex.pages[pg_num];
#if LAZY_PAGE_ALLOC == 1
  // lazy page allocation, fill the page with default value
  if (flexCols_[colID].old_pages[pg_num] == nullptr) {
    gart::util::lock32(&flex.locks[pg_num]);
    if (flexCols_[colID].old_pages[pg_num] == nullptr) {
      page = getNewPage_(col.page_size, col.vlen, col.real_column_num, -1,
                         nullptr, colID, pg_num);
      flexCols_[colID].old_pages[pg_num] = page;
    }
    gart::util::unlock32(&flex.locks[pg_num]);
  }
#endif
  if (page->ver == -1) {
    page->ver = version;  // a initialized page
    page->min_ver = version;
    flex.pages[pg_num] = page;
  } else if (version > page->ver) {
    gart::util::lock32(&flex.locks[pg_num]);
    page = flex.pages[pg_num];

    if (version > page->ver) {
      Page* newPage = getNewPage_(col.page_size, col.vlen, col.real_column_num,
                                  version, page, colID, pg_num);
      flex.pages[pg_num] = newPage;
      page = newPage;
    }

    gart::util::unlock32(&flex.locks[pg_num]);
  }

  assert(page && page->ver == version);
  return page;
}

inline PropertyColPaged::Page* PropertyColPaged::findPage(int colID,
                                                          uint64_t pg_num,
                                                          uint64_t version,
                                                          uint64_t* walk_cnt) {
  const Property::Column& col = cols_[colID];

  FlexCol& flex = flexCols_[colID];
  assert(pg_num < flex.pages.size());
  Page* page = flex.pages[pg_num];
  if (page == nullptr || page->min_ver > version) {
    // assert(false);
    return nullptr;
  }

  for (; page; page = page->prev) {
    if (page->ver <= version) {
      assert(page);
      // if (walk_cnt) ++(*walk_cnt);
      if (version == 0)
        assert(page == flexCols_[colID].pages[pg_num]);
      return page;
    }
    if (walk_cnt)
      ++(*walk_cnt);
  }

  assert(false);
  return nullptr;
}

void PropertyColPaged::insert(uint64_t off, uint64_t k, char* v, uint64_t ver) {
  if (unlikely(off > max_items_)) {
    LOG(ERROR) << "off " << off << " > max_items_ " << max_items_;
    assert(false);
  }

  for (int i = 0; i < cols_.size(); ++i) {
    const Property::Column& col = cols_[i];

    size_t vlen = col.vlen;
    char* dst = nullptr;
    Page* page;
    if (col.updatable) {
      int pg_num = off / col.page_size;
      page = findWithInsertPage_(i, pg_num, ver);
      assert(page && page->ver == ver);
      dst = page->content + BYTE_SIZE(col.page_size * col.real_column_num) +
            (off % col.page_size) * vlen;
    } else {
      dst = fixCols_[i] + off * vlen;
    }
    copy_val_(dst, v, vlen);

#if UPDATE_STAT
    ++stat_.num_update;
#endif
    v += vlen;  // XXX: FIXME! for the length of value
  }
}

void PropertyColPaged::insert(uint64_t off, uint64_t k,
                              const StringViewList& v_list, uint64_t ver,
                              gart::graph::GraphStore* graph_store) {
  GART_ASSERT(off < max_items_);

  auto graph_schema = graph_store->get_schema();

  std::vector<char*> prop_buffer(cols_.size(), nullptr);
  std::vector<std::vector<bool>> prop_value_is_null(cols_.size());
  for (auto idx = 0; idx < cols_.size(); idx++) {
    prop_buffer[idx] = (char*) malloc(cols_[idx].vlen);
  }

  for (auto prop_idx = 0; prop_idx < v_list.size(); prop_idx++) {
    auto col_family_id =
        graph_store->get_vertex_prop_column_family_map(table_id_, prop_idx);
    size_t col_family_offset =
        graph_store->get_vertex_prop_offset_in_column_family(table_id_,
                                                             prop_idx);
    auto dtype = graph_schema.dtype_map[std::make_pair(table_id_, prop_idx)];
    if (dtype != STRING && v_list[prop_idx].size() == 0) {
      prop_value_is_null[col_family_id].push_back(true);
      continue;
    } else if (dtype == STRING &&
               (std::stoll(std::string(v_list[prop_idx])) & 0xffff) == 0) {
      prop_value_is_null[col_family_id].push_back(true);
      continue;
    }
    prop_value_is_null[col_family_id].push_back(false);
    if (dtype == INT) {
      *((int*) (prop_buffer[col_family_id] + col_family_offset)) =
          std::stoi(std::string(v_list[prop_idx]));
    } else if (dtype == LONG) {
      *((int64_t*) (prop_buffer[col_family_id] + col_family_offset)) =
          std::stoll(std::string(v_list[prop_idx]));
    } else if (dtype == DOUBLE) {
      *((double*) (prop_buffer[col_family_id] + col_family_offset)) =
          std::stod(std::string(v_list[prop_idx]));
    } else if (dtype == FLOAT) {
      *((float*) (prop_buffer[col_family_id] + col_family_offset)) =
          std::stof(std::string(v_list[prop_idx]));
    } else if (dtype == STRING) {
      int64_t value = std::stoll(std::string(v_list[prop_idx]));
      *((int64_t*) (prop_buffer[col_family_id] + col_family_offset)) =
          std::stoll(std::string(v_list[prop_idx]));
    } else {
      LOG(ERROR) << "Unsupported data type: " << dtype;
      assert(false);
    }
  }

  for (auto idx = 0; idx < cols_.size(); idx++) {
    const Property::Column& col = cols_[idx];
    size_t vlen = col.vlen;
    char* dst = nullptr;
    Page* page;
    if (col.updatable) {
      int pg_num = off / col.page_size;
      page = findWithInsertPage_(idx, pg_num, ver);
      assert(page && page->ver == ver);
      dst = page->content + BYTE_SIZE(col.page_size * col.real_column_num) +
            (off % col.page_size) * vlen;
    } else {
      dst = fixCols_[idx] + off * vlen;
    }
    memcpy(dst, prop_buffer[idx], vlen);
    free(prop_buffer[idx]);
    for (auto prop_idx = 0; prop_idx < prop_value_is_null[idx].size();
         prop_idx++) {
      if (prop_value_is_null[idx][prop_idx] == false) {
        reset_bit((uint8_t*) (page->content),
                  off % col.page_size * col.real_column_num + prop_idx);
      } else {
        set_bit((uint8_t*) (page->content),
                off % col.page_size * col.real_column_num + prop_idx);
      }
    }
  }
}

void PropertyColPaged::update(uint64_t off, uint64_t k,
                              const StringViewList& v_list, uint64_t ver,
                              gart::graph::GraphStore* graph_store) {
  std::vector<char*> prop_buffer(cols_.size(), nullptr);
  std::vector<bool> col_family_updated(cols_.size(), false);
  std::vector<std::vector<bool>> prop_value_is_null(cols_.size());
  for (auto col_family_id = 0; col_family_id < cols_.size(); col_family_id++) {
    prop_buffer[col_family_id] = (char*) malloc(cols_[col_family_id].vlen);
  }

  auto graph_schema = graph_store->get_schema();

  for (auto prop_idx = 0; prop_idx < v_list.size(); prop_idx++) {
    auto col_family_id =
        graph_store->get_vertex_prop_column_family_map(table_id_, prop_idx);
    auto col_family_offset =
        graph_store->get_vertex_prop_offset_in_column_family(table_id_,
                                                             prop_idx);
    auto prop_id_in_col_family =
        graph_store->get_vertex_prop_id_in_column_family(table_id_, prop_idx);
    bool is_null_value = false;
    bool old_value_is_null = false;
    const Property::Column& col = cols_[col_family_id];
    size_t vlen = col.vlen;
    int pg_num = off / col.page_size;
    FlexCol& flex = flexCols_[col_family_id];
    Page* page = flex.pages[pg_num];

    if (get_bit((uint8_t*) (page->content),
                off % col.page_size * col.real_column_num +
                    prop_id_in_col_family) == true) {
      old_value_is_null = true;
    }

    if (v_list[prop_idx].size() == 0) {
      is_null_value = true;
      prop_value_is_null[col_family_id].push_back(true);
      if (!old_value_is_null) {
        col_family_updated[col_family_id] = true;
      }
      continue;
    }
    prop_value_is_null[col_family_id].push_back(false);

    auto dtype = graph_schema.dtype_map[std::make_pair(table_id_, prop_idx)];
    char* dst = page->content + BYTE_SIZE(col.page_size * col.real_column_num) +
                (off % col.page_size) * vlen + col_family_offset;
    if (dtype == INT) {
      int new_value = std::stoi(std::string(v_list[prop_idx]));
      *((int*) (prop_buffer[col_family_id] + col_family_offset)) = new_value;
      if (old_value_is_null || new_value != *((int*) dst)) {
        col_family_updated[col_family_id] = true;
      }
    } else if (dtype == LONG) {
      int64_t new_value = std::stoll(std::string(v_list[prop_idx]));
      *((int64_t*) (prop_buffer[col_family_id] + col_family_offset)) =
          new_value;
      if (old_value_is_null || new_value != *((int64_t*) dst)) {
        col_family_updated[col_family_id] = true;
      }
    } else if (dtype == DOUBLE) {
      double new_value = std::stod(std::string(v_list[prop_idx]));
      *((double*) (prop_buffer[col_family_id] + col_family_offset)) = new_value;
      if (old_value_is_null || new_value != *((double*) dst)) {
        col_family_updated[col_family_id] = true;
      }
    } else if (dtype == FLOAT) {
      float new_value = std::stof(std::string(v_list[prop_idx]));
      *((float*) (prop_buffer[col_family_id] + col_family_offset)) = new_value;
      if (old_value_is_null || new_value != *((float*) dst)) {
        col_family_updated[col_family_id] = true;
      }
    } else if (dtype == STRING) {
      std::string old_value;
      if (!old_value_is_null) {
        int64_t old_str_key = *(int64_t*) dst;
        graph_store->get_string(old_str_key, old_value);
      }
      std::string new_value = std::string(v_list[prop_idx]);
      if (old_value_is_null || old_value != new_value) {
        uint64_t new_str_key = graph_store->put_cstring(new_value);
        *((int64_t*) (prop_buffer[col_family_id] + col_family_offset)) =
            new_str_key;
        col_family_updated[col_family_id] = true;
      } else {
        *((int64_t*) (prop_buffer[col_family_id] + col_family_offset)) =
            *(int64_t*) dst;
      }
    } else {
      LOG(ERROR) << "Unsupported data type: " << dtype;
      assert(false);
    }
  }

  for (auto col_family_id = 0; col_family_id < cols_.size(); col_family_id++) {
    if (col_family_updated[col_family_id]) {
      const Property::Column& col = cols_[col_family_id];
      size_t vlen = col.vlen;
      int pg_num = off / col.page_size;
      Page* new_page = findWithInsertPage_(col_family_id, pg_num, ver);
      assert(new_page && new_page->ver == ver);
      char* new_dst = new_page->content + (off % col.page_size) * vlen +
                      BYTE_SIZE(col.page_size * col.real_column_num);
      memcpy(new_dst, prop_buffer[col_family_id], vlen);
      for (auto prop_id = 0; prop_id < prop_value_is_null[col_family_id].size();
           prop_id++) {
        if (prop_value_is_null[col_family_id][prop_id] == false) {
          reset_bit((uint8_t*) (new_page->content),
                    off % col.page_size * col.real_column_num + prop_id);
        } else {
          set_bit((uint8_t*) (new_page->content),
                  off % col.page_size * col.real_column_num + prop_id);
        }
      }
    }
    free(prop_buffer[col_family_id]);
  }
}

void PropertyColPaged::update(uint64_t off, const vector<int>& cids, char* v,
                              uint64_t seq, uint64_t ver) {
  if (val_len_ == 0)
    return;
  assert(off < max_items_);

  // int64_t &meta_seq = seq_[off];
  // assert(meta_seq != -1 && meta_seq != seq);

  // if (meta_seq >= seq) return;  // XXX: fix it, set seq on each columns

  for (int i : cids) {
    const Property::Column& col = cols_[i];

    uint64_t vlen = col.vlen;
    uint64_t voff = val_off_[i];

    char* dst = nullptr;
    assert(col.updatable);

    int pg_num = off / col.page_size;

    Page* page = findWithInsertPage_(i, pg_num, ver);
    assert(page && page->ver == ver);
    dst =
        page->content + (off % col.page_size) * vlen + BYTE_SIZE(col.page_size);
    memcpy(dst, &v[voff], vlen);

#if UPDATE_STAT
    ++stat_.num_update;
#endif
  }

  // meta_seq = seq;
}

void PropertyColPaged::update(uint64_t off, int cid, char* v, uint64_t ver) {
  if (val_len_ == 0)
    return;
  assert(off < max_items_);

  const Property::Column& col = cols_[cid];

  uint64_t vlen = col.vlen;

  char* dst = nullptr;
  assert(col.updatable);

  int pg_num = off / col.page_size;

  Page* page = findWithInsertPage_(cid, pg_num, ver);
  assert(page && page->ver == ver);
  dst = page->content + (off % col.page_size) * vlen + BYTE_SIZE(col.page_size);
  memcpy(dst, v, vlen);

#if UPDATE_STAT
  ++stat_.num_update;
#endif
}

void PropertyColPaged::gc(uint64_t ver) {
  if (ver == 0)
    return;

  uint64_t clean_sz = 0;
  uint64_t clean_pg = 0;

  // uint64_t header = header_;
  uint64_t header = stable_header_;
  for (int i = 0; i < cols_.size(); i++) {
    if (!cols_[i].updatable)
      continue;
    vector<Page*>& old_pages = flexCols_[i].old_pages;
    int pgsz = cols_[i].page_size;
    size_t vlen = cols_[i].vlen;
    int used_page = (header + pgsz - 1) / pgsz;
    assert(used_page <= old_pages.size());
    for (int pi = 0; pi < used_page; ++pi) {
      Page* p = old_pages[pi];
      if (p->ver >= ver)
        continue;

      while (p->ver < ver && p->next && p->next->ver <= ver) {
        Page* next = p->next;
        next->prev = nullptr;
        free(p);
        p = next;
        clean_sz += sizeof(Page) + vlen * pgsz;
        ++clean_pg;
      }
      old_pages[pi] = p;
    }
  }
#if 0
  if (clean_sz != 0) {
    printf("GC ver %lu table_id %d page %lu, size %lu\n",
           ver, table_id_, clean_pg, clean_sz);
  }
#endif
}

char* PropertyColPaged::getByOffset(uint64_t offset, int col_id,
                                    uint64_t version, uint64_t* walk_cnt) {
  char* val = nullptr;
  const Property::Column& col = cols_[col_id];
  assert(offset < max_items_);
  // if (offset >= max_items_) return nullptr;

  Page* page = nullptr;
  if (col.updatable) {
    uint64_t pg_num = offset / col.page_size;
#if 1
    if (unlikely(table_id_ >= MAX_CACHED_TABLES && col_id >= MAX_CACHED_COLS)) {
      page = findPage(col_id, pg_num, version, walk_cnt);
    } else {
      if (cached_page[table_id_][col_id] != nullptr &&
          cached_ver[table_id_] == version &&
          cached_page_num[table_id_][col_id] == pg_num) {
        page = reinterpret_cast<Page*>(cached_page[table_id_][col_id]);
      } else {
        cached_ver[table_id_] = version;
        page = findPage(col_id, pg_num, version, walk_cnt);
        cached_page[table_id_][col_id] = page;
        cached_page_num[table_id_][col_id] = pg_num;
      }
    }
#else
    page = findPage(col_id, pg_num, version, walk_cnt);
#endif

    if (!page)
      return nullptr;
    assert(page == flexCols_[col_id].pages[0]);
    val = page->content + (offset % col.page_size) * col.vlen;
  } else {
    val = fixCols_[col_id] + col.vlen * offset;
  }

  return val;
}

size_t PropertyColPaged::getCol(int col_id, uint64_t start_off, size_t size,
                                uint64_t lver, vector<char*>& pages) {
  const Property::Column& col = cols_[col_id];
  pages.clear();
  assert(col.updatable);
  int start_pg = start_off / col.page_size;
  int end_pg = (start_off + size + col.page_size - 1) / col.page_size;
  for (int i = start_pg; i < end_pg; i++) {
    Page* p = findPage(col_id, i, lver);
    assert(p != nullptr);
    pages.push_back(p->content);
  }
  return col.page_size;
}

size_t PropertyColPaged::getPageSize(int col_id) const {
  return cols_[col_id].page_size;
}

char* PropertyColPaged::locateValue(int cid, char* col, size_t offset) {
  size_t vlen = cols_[cid].vlen;
  size_t voff = offset % cols_[cid].page_size;
  return (col + vlen * voff);
}

PropertyColPaged::Cursor::Cursor(const PropertyColPaged& store, int col_id,
                                 uint64_t ver)
    : col_(store),
      col_id_(col_id),
      ver_(ver),
      update_(store.cols_[col_id].updatable),
      vlen_(store.cols_[col_id].vlen),
      pgsz_(store.cols_[col_id].page_size),
      begin_(0),
      end_(store.stable_header_),
      cur_(-1),
      ptr_(nullptr),
      base_(update_ ? nullptr : store.fixCols_[col_id]),
      pgn_(-1),
      pgi_(pgsz_ - 1),
      pages_(store.flexCols_[col_id].pages) {}

void PropertyColPaged::Cursor::seekOffset(uint64_t begin, uint64_t end) {
  uint64_t header = col_.stable_header_;
  if (end > header)
    end = header;
  if (begin > end)
    begin = end;
  begin_ = begin;
  end_ = end;
  cur_ = begin_ - 1;

  if (update_) {
    pgi_ = begin_ % pgsz_ - 1;
    if (pgi_ == -1) {
      pgi_ = pgsz_ - 1;
      pgn_ = begin_ / pgsz_ - 1;
    } else {
      pgn_ = begin_ / pgsz_;
    }
  }
}

#if 1  // 1 for MVCS, 0 for pure col
bool PropertyColPaged::Cursor::nextRow(uint64_t* walk_cnt) {
  ++cur_;
  if (cur_ >= end_)
    return false;
    // if (col_.min_ver_[cur_] > ver_) return false;  // for cache corherence

#if 1
  if (unlikely(!update_)) {
    ptr_ = base_ + vlen_ * cur_;
    return true;
  }

  ++pgi_;
  if (pgi_ == pgsz_) {
    pgi_ = 0;
    ++pgn_;
  }

  if (pgi_ == 0 || base_ == nullptr) {
    // relocate page
    Page* p = pages_[pgn_];
    if (p->min_ver > ver_)
      return false;

    for (; p != nullptr; p = p->prev) {
      if (walk_cnt)
        ++(*walk_cnt);
      if (p->ver <= ver_)
        break;
    }
    assert(p);
    base_ = p->content;
  }
  ptr_ = base_ + vlen_ * pgi_;
#else
    // if (!ptr_)
    //   ptr_ = pages_[0]->content;
#endif
  return true;
}
#else
bool PropertyColPaged::Cursor::nextRow(uint64_t* walk_cnt) {
  ++cur_;
  if (unlikely(cur_ >= end_))
    return false;

  if (unlikely(!update_)) {
    ptr_ = base_ + vlen_ * cur_;
    return true;
  }

  ++pgi_;
  if (unlikely(pgi_ == pgsz_)) {
    // pgi_ = 0;
    ++pgn_;
  }

  if (base_ == nullptr) {
    Page* p = pages_[0];
    base_ = p->content;
  }

  ptr_ = base_ + vlen_ * cur_;
  return true;
}
#endif

}  // namespace property
}  // namespace gart
