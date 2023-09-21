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

#ifndef VEGITO_SRC_PROPERTY_PROPERTY_COL_PAGED_H_
#define VEGITO_SRC_PROPERTY_PROPERTY_COL_PAGED_H_

#include <memory>
#include <vector>

#include "property/property.h"

namespace gart {
namespace property {

class PropertyColPaged : public Property {
 public:
  PropertyColPaged(Property::Schema schema, uint64_t max_items,
                   memory::BufferManager& buf_mgr);
  ~PropertyColPaged();

  // for insert
  void insert(uint64_t off, uint64_t k, char* v, uint64_t ver) override;

  void insert(uint64_t off, uint64_t k, const StringViewList& v_list,
              uint64_t ver, gart::graph::GraphStore* graph_store) override;

  // get cursor
  std::unique_ptr<ColCursor> getColCursor(int col_id,
                                          uint64_t ver) const override {
    ColCursor* c = new Cursor(*this, col_id, ver);
    return std::unique_ptr<ColCursor>(c);
  }

  char* getByOffset(uint64_t offset, int columnID, uint64_t version,
                    uint64_t* walk_cnt = nullptr) override;

  void gc(uint64_t version) override;

  char* col(int col_id, uint64_t* len = nullptr) const override {
    assert(!cols_[col_id].updatable);
    if (len)
      *len = header_;
    return fixCols_[col_id];
  }

  void update(uint64_t off, uint64_t k, const StringViewList& v_list,
              uint64_t ver, gart::graph::GraphStore* graph_store) override;

  void update(uint64_t off, const std::vector<int>& cids, char* v, uint64_t seq,
              uint64_t ver) override;

  void update(uint64_t off, int cid, char* v, uint64_t ver) override;

  const std::vector<uint64_t>& getKeyCol() const;

  // return pagesize of the col
  size_t getCol(int col_id, uint64_t start_off, size_t size, uint64_t lver,
                std::vector<char*>& pages);
  size_t getPageSize(int col_id) const;
  char* locateValue(int colid, char* col, size_t offset);

 private:
  struct Page {
    uint64_t ver;
    uintptr_t prev_ptr;
    uint64_t v6d_offset;
    uint64_t min_ver;
    Page* prev;
    Page* next;
    char content[0];

    Page() {}

    Page(uint64_t v, uint64_t o, Page* n)
        : ver(v),
          v6d_offset(o),
          prev(n),
          next(nullptr),
          prev_ptr(0),
          min_ver(n ? n->min_ver : v) {
      if (n) {
        n->next = this;
      }
    }
  };

  struct FlexCol {
    std::vector<Page*> pages;  // header (newest)
    std::vector<uint32_t> locks;
    std::vector<Page*> old_pages;  // tailer (oldest)
  };

  Page* getNewPage_(uint64_t page_sz, uint64_t vlen, uint64_t real_column_num,
                    uint64_t ver, Page* prev, uint64_t prop_id,
                    uint64_t pg_num);

  Page* findWithInsertPage_(int col_id, uint64_t page_num, uint64_t version);

  const int table_id_;
  uint64_t val_len_;

  // for each column
  std::vector<Property::ColumnFamily> cols_;

  std::vector<char*> fixCols_;
  std::vector<FlexCol> flexCols_;

  // stored in Blob
  struct FlexColHeader {
    int num_row_per_page;
    uintptr_t page_ptr[0];

    static size_t size(int num_pages) {
      return sizeof(FlexColHeader) + num_pages * sizeof(uintptr_t);
    }
  };

  struct FlexBuf {
    char* buf = nullptr;
    size_t allocated_sz = 0;
    size_t total_sz;
    FlexColHeader* header;
  };

  std::vector<FlexBuf> flex_bufs_;

  // object id for each property
  std::vector<vineyard::ObjectID> col_v6d_oids_;
  std::vector<uint64_t> col_v6d_offsets_;  // for FlexColHeader

  Page* findPage(int col_id, uint64_t page_num, uint64_t version,
                 uint64_t* walk_cnt = nullptr);

 public:
  class Cursor : public Property::ColCursor {
   public:
    Cursor(const PropertyColPaged& store, int col_id, uint64_t ver);
    void seekOffset(uint64_t begin, uint64_t end) override;
    inline bool nextRow(uint64_t* walk_cnt = nullptr) override;
    char* value() const override { return ptr_; }

    uint64_t cur() const override { return cur_; }
    char* base() const override {
      assert(!update_);
      return base_;
    }

   private:
    const PropertyColPaged& col_;
    const uint64_t ver_;
    const int col_id_;

    const bool update_;
    const uint64_t vlen_;
    const uint64_t pgsz_;  // in items

    uint64_t begin_;
    uint64_t end_;
    uint64_t cur_;
    char* ptr_;
    char* base_;

    uint64_t pgn_;                     // for updatable columns
    uint64_t pgi_;                     // for updatable columns
    const std::vector<Page*>& pages_;  // for updatable columns
  };

  friend class Cursor;
};

}  // namespace property
}  // namespace gart

#endif  // VEGITO_SRC_PROPERTY_PROPERTY_COL_PAGED_H_
