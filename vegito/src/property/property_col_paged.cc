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

#include "property/property_col_paged.h"
#include <cstdint>
#include <string>

#include "graph/graph_store.h"
#include "util/bitset.h"
#include "util/macros.h"

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

// NOTE: page_sz is number of objects, instead of bytes
inline PropertyColPaged::Page* PropertyColPaged::getNewPage_(uint64_t page_sz,
                                                             uint64_t vlen,
                                                             uint64_t ver,
                                                             Page* prev) {
  char* buf = nullptr;
  uint32_t pg_sz = sizeof(Page) + vlen * page_sz;

  buf = reinterpret_cast<char*>(malloc(pg_sz));
  Page* ret = new (buf) Page(ver, prev);

  if (page_sz != 1 && prev != nullptr) {
    memcpy(ret->content, prev->content, page_sz * vlen);
#if UPDATE_STAT
    stat_.num_copy += vlen * page_sz;  // size
                                       // ++stat_.num_copy;    // count
#endif
  }

  return ret;
}

inline PropertyColPaged::Page* PropertyColPaged::getNewPage_(
    uint64_t page_sz, uint64_t vlen, uint64_t ver, Page* prev, uint64_t prop_id,
    uint64_t pg_num) {
  FlexBuf& flex_buf = flex_bufs_[prop_id];
  char* buf = nullptr;
  uint32_t pg_sz = sizeof(Page) + vlen * page_sz;

  buf = reinterpret_cast<char*>(&flex_buf.buf[flex_buf.allocated_sz]);
  uintptr_t cur_ptr = flex_buf.allocated_sz;
  flex_buf.allocated_sz += pg_sz;
  assert(flex_buf.allocated_sz <= flex_buf.total_sz);
  Page* ret = new (buf) Page(ver, prev);

  if (prev != nullptr) {
    ret->prev_ptr =
        reinterpret_cast<char*>(prev) - reinterpret_cast<char*>(flex_buf.buf);
  }

  if (page_sz != 1 && prev != nullptr) {
    memcpy(ret->content, prev->content, page_sz * vlen);
#if UPDATE_STAT
    stat_.num_copy += vlen * page_sz;  // size
                                       // ++stat_.num_copy;    // count
#endif
  }
  flex_buf.header->page_ptr[pg_num] = cur_ptr;
  return ret;
}

inline PropertyColPaged::Page* PropertyColPaged::getInitPage_(uint64_t page_sz,
                                                              uint64_t vlen,
                                                              uint64_t prop_id,
                                                              uint64_t pg_num) {
  FlexBuf& flex_buf = flex_bufs_[prop_id];
  char* buf = nullptr;
  uint32_t pg_sz = sizeof(Page) + vlen * page_sz;

  buf = reinterpret_cast<char*>(&flex_buf.buf[flex_buf.allocated_sz]);
  uintptr_t cur_ptr = flex_buf.allocated_sz;
  flex_buf.allocated_sz += pg_sz;

  // must have empty space for multi-version pages (getNewPage_), so not <=
  assert(flex_buf.allocated_sz < flex_buf.total_sz);

  flex_buf.header->page_ptr[pg_num] = cur_ptr;
  return reinterpret_cast<Page*>(buf);
}

PropertyColPaged::PropertyColPaged(Property::Schema s, uint64_t max_items,
                                   size_t bitmap_size,
                                   const vector<uint32_t>* split)
    : Property(max_items),
      table_id_(s.table_id),
      cols_(s.cols),
      col_oids_(s.cols.size()),
      flex_bufs_(s.cols.size()),
      fixCols_(s.cols.size(), nullptr),
      flexCols_(s.cols.size()),
      null_bitmap_size_(bitmap_size) {
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

  null_bitmaps_ = reinterpret_cast<uint8_t*>(
      mem_alloc(null_bitmap_size_ * max_items_, &row_meta_oid_));

  memset(null_bitmaps_, 0, null_bitmap_size_ * max_items_);

  for (int i = 0; i < cols_.size(); ++i) {
    size_t vlen = val_lens_[i];
    // divided into 2 types according to "updatable"
    cols_[i].updatable = true;
    if (cols_[i].updatable) {
      size_t page_sz = cols_[i].page_size;
      int page_num = (max_items_ + page_sz - 1) / page_sz;
      assert(page_num != 0);
      flexCols_[i].locks.assign(page_num, 0);
      flexCols_[i].pages.assign(page_num, nullptr);
      flexCols_[i].old_pages.assign(page_num, nullptr);
      size_t total_sz = FlexColHeader::size(page_num) +
                        page_num * (sizeof(Page) + page_sz * vlen);
      // TODO(ssj): (hardcode) 1.5 is 1x init pages + 0.5x MVCC pages
      total_sz *= 1.5;
      // printf(
      //     "Vlabel %d column %d (flex), "
      //     "page size %lu, vlen %lu, page num %d, size of header %lu, "
      //     "malloc %lf GB\n",
      //     table_id_, i, page_sz, vlen, page_num,
      //     FlexColHeader::size(page_num), total_sz / 1024.0 / 1024 / 1024);
      char* buf = mem_alloc(total_sz, &col_oids_[i]);
      FlexColHeader* header = reinterpret_cast<FlexColHeader*>(buf);
      flex_bufs_[i].total_sz = total_sz;
      flex_bufs_[i].buf = buf;
      flex_bufs_[i].header = header;
      flex_bufs_[i].allocated_sz = 0 + FlexColHeader::size(page_num);

      header->num_row_per_page = page_sz;

      for (int p = 0; p < page_num; ++p) {
#if LAZY_PAGE_ALLOC == 0
        Page* page = getNewPage_(page_sz, vlen, -1, nullptr, i, p);
        flexCols_[i].old_pages[p] = page;
#else
        Page* page = getInitPage_(page_sz, vlen, i, p);
#endif
        flexCols_[i].pages[p] = page;
      }
    } else {
      fixCols_[i] = mem_alloc(vlen * max_items_, &col_oids_[i]);
      printf("Vlabel %d column %d (fixed), malloc %lf GB\n", table_id_, i,
             vlen * max_items_ / 1024.0 / 1024 / 1024);
    }
  }

  blob_metas_.resize(cols_.size());
  for (int i = 0; i < cols_.size(); ++i) {
    gart::VPropMeta& meta = blob_metas_[i];
    meta.init(i, val_lens_[i], cols_[i].updatable, cols_[i].vtype);

    meta.set_oid(col_oids_[i], 0);  // TODO(wanglei): hard code of header
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
      page = new (page) Page(version, nullptr);
      flexCols_[colID].old_pages[pg_num] = page;
    }
    gart::util::unlock32(&flex.locks[pg_num]);
  }
#endif
  if (page->ver == -1) {
    page->ver = version;  // a initialized page
    page->min_ver = version;
  } else if (version > page->ver) {
    gart::util::lock32(&flex.locks[pg_num]);
    page = flex.pages[pg_num];

    if (version > page->ver) {
      // TODO(wanglei): update pages on vineyard
      Page* newPage =
          getNewPage_(col.page_size, col.vlen, version, page, colID, pg_num);
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
    if (col.updatable) {
      int pg_num = off / col.page_size;

      Page* page = findWithInsertPage_(i, pg_num, ver);
      assert(page && page->ver == ver);
      dst = page->content + (off % col.page_size) * vlen;
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
                              const StringViewList& v_list, uint64_t ver) {
  if (unlikely(off > max_items_)) {
    LOG(ERROR) << "off " << off << " > max_items_ " << max_items_;
    assert(false);
  }

  assert(v_list.size() == cols_.size() || (v_list.size() + 1 == cols_.size()));

  char* prop_buffer;
  bool enable_row = false;
  size_t buffer_offset = null_bitmap_size_;
  if (v_list.size() + 1 == cols_.size()) {
    prop_buffer = (char*) malloc(cols_[cols_.size() - 1].vlen);
    enable_row = true;
  }

  for (int i = 0; i < cols_.size(); ++i) {
    const Property::Column& col = cols_[i];

    size_t vlen = col.vlen;
    char* dst = nullptr;
    if (col.updatable) {
      int pg_num = off / col.page_size;

      Page* page = findWithInsertPage_(i, pg_num, ver);
      assert(page && page->ver == ver);
      dst = page->content + (off % col.page_size) * vlen;
    } else {
      dst = fixCols_[i] + off * vlen;
    }

    if (i < v_list.size() && v_list[i].size() == 0) {
      set_bit(null_bitmaps_ + null_bitmap_size_ * off, i);
    } else if (i < v_list.size() && col.vtype == STRING &&
               (std::stoll(std::string(v_list[i])) & 0xffff) == 0) {
      set_bit(null_bitmaps_ + null_bitmap_size_ * off, i);
    } else if (i < v_list.size()) {
      assign_prop(col.vtype, dst, v_list[i]);
      if (enable_row) {
        assign_prop(col.vtype, prop_buffer + buffer_offset, v_list[i]);
        buffer_offset += vlen;
      }
    } else {
      assert(col.vtype == BYTES);
      memcpy(prop_buffer, null_bitmaps_ + null_bitmap_size_ * off,
             null_bitmap_size_);
      memcpy(dst, prop_buffer, vlen);
      free(prop_buffer);
    }

#if UPDATE_STAT
    ++stat_.num_update;
#endif
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
    dst = page->content + (off % col.page_size) * vlen;
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
  dst = page->content + (off % col.page_size) * vlen;
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
