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

#ifndef VEGITO_SRC_PROPERTY_PROPERTY_COL_ARRAY_H_
#define VEGITO_SRC_PROPERTY_PROPERTY_COL_ARRAY_H_

#include <memory>
#include <vector>

#include "property/property.h"

namespace gart {
namespace property {

class PropertyColArray : public Property {
 public:
  explicit PropertyColArray(Property::Schema schema, uint64_t max_items,
                            memory::BufferManager& buf_mgr);

  void insert(uint64_t off, uint64_t k, char* v, uint64_t ver) override;

  // get cursor
  std::unique_ptr<ColCursor> getColCursor(int col_id,
                                          uint64_t ver) const override {
    ColCursor* c = new Cursor(*this, col_id, ver);
    return std::unique_ptr<ColCursor>(c);
  }

  char* getByOffset(uint64_t offset, uint64_t version) override;
  char* getByOffset(uint64_t offset, int columnID, uint64_t version,
                    uint64_t* walk_cnt = nullptr) override;

  char* col(int col_id, uint64_t* len = nullptr) const override {
    assert(!cols_[col_id].updatable);
    if (len)
      *len = header_;
    return fixCols_[col_id];
  }

  void update(uint64_t off, const std::vector<int>& cids, char* v, uint64_t seq,
              uint64_t ver);

  size_t getItemNum(uint64_t lver) const;
  const std::vector<uint64_t>& getKeyCol() const;
  char* getFixCol(int col_id) const;

 private:
  // for insert or update
  void _put(uint64_t offset, uint64_t key, char* val, int64_t seq,
            uint64_t version, bool insert);

  struct ValueNode {
    uint64_t ver;
    ValueNode* prev;
    char val[0];
  };

  inline uint64_t get_field_width_(int col_id) const {
    uint64_t vlen = cols_[col_id].vlen;
    if (cols_[col_id].updatable)
      vlen += sizeof(ValueNode);
    return vlen;
  }

  std::vector<Property::ColumnFamily> cols_;
  std::vector<int64_t> seq_;

  std::vector<uint64_t> key_col_;
  std::vector<char*> fixCols_;
  std::vector<char*> flexCols_;

 public:
  class Cursor : public Property::ColCursor {
   public:
    Cursor(const PropertyColArray& store, int col_id, uint64_t ver);
    void seekOffset(uint64_t begin, uint64_t end) override;
    bool nextRow(uint64_t* walk_cnt = nullptr) override;
    char* value() const override;
    uint64_t cur() const override { return cur_; }
    char* base() const override { return base_; }

    uint64_t key() const { return col_.key_col_[cur_]; }
    uint64_t length() const { return col_.header_; }

   private:
    const PropertyColArray& col_;
    const uint64_t ver_;

    const bool update_;
    const int col_id_;
    char* const base_;
    const uint64_t field_width_;  // in bytes

    uint64_t begin_;
    uint64_t end_;
    uint64_t cur_;
    char* ptr_;
  };

  friend class Cursor;
};

}  // namespace property
}  // namespace gart

#endif  // VEGITO_SRC_PROPERTY_PROPERTY_COL_ARRAY_H_
