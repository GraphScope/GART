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

#ifndef VEGITO_SRC_PROPERTY_PROPERTY_H_
#define VEGITO_SRC_PROPERTY_PROPERTY_H_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "glog/logging.h"
#include "util/macros.h"
#include "vineyard/common/util/uuid.h"

#include "fragment/shared_storage.h"
#include "graph/type_def.h"
#include "seggraph/allocator.hpp"
#include "util/util.h"

namespace gart {

namespace graph {
class GraphStore;
}  // namespace graph

namespace property {

typedef std::vector<std::string_view> StringViewList;

enum PropertyStoreType { /* PROP_KV, PROP_ROW, */
                         PROP_COLUMN = 2,
                         PROP_COLUMN2
};

enum PropertyStoreDataType {
  INVALID = 0,
  BOOL = 1,
  CHAR = 2,
  SHORT = 3,
  INT = 4,
  LONG = 5,
  FLOAT = 6,
  DOUBLE = 7,
  STRING = 8,
  BYTES = 9,
  INT_LIST = 10,
  LONG_LIST = 11,
  FLOAT_LIST = 12,
  DOUBLE_LIST = 13,
  STRING_LIST = 14,
  DATE = 15,
  DATETIME = 16,
  TIME = 17,
  TIMESTAMP = 18
};

// multi-version store
class Property {  // NOLINT(build/class)
 public:
  struct Column {
    size_t vlen;
    bool updatable;
    size_t page_size;  // uint64_t(-1) or 0 means infinity, unit: items
    PropertyStoreDataType vtype = FLOAT;
    size_t real_column_num = 0;
  };

  // schema for one type of vertex/edge (including serveral columns)
  struct Schema {
    int table_id;
    size_t klen;
    std::vector<Column> cols;  //  each column
    PropertyStoreType store_type;
  };

  // ATTENTION: add lock in the index level!
  virtual void insert(uint64_t off, uint64_t k, char* v, uint64_t ver) {
    assert(false);
  }

  virtual void insert(uint64_t off, uint64_t k, const StringViewList& v_list,
                      uint64_t ver, gart::graph::GraphStore* graph_store) {
    assert(false);
  }

  virtual void update(uint64_t off, uint64_t k, const StringViewList& v_list,
                      uint64_t ver, gart::graph::GraphStore* graph_store) {
    assert(false);
  }

  virtual void update(uint64_t off, const std::vector<int>& cids, char* v,
                      uint64_t seq, uint64_t ver) {
    assert(false);
  }

  virtual void update(uint64_t off, int cid, char* v, uint64_t ver) {
    assert(false);
  }

  uint64_t copy(const Property* store) {
    uint64_t max_ver = uint64_t(-1);
    auto row_cursor = store->getRowCursor(max_ver);
    assert(row_cursor.get());

    uint64_t off = 0;
    for (; row_cursor->nextRow(); ++off) {
      this->insert(off, row_cursor->key(), row_cursor->value(), 2);
    }

    return off;
  }

  char* mem_alloc(size_t bytes, uint64_t* obj_id = nullptr) {
    return array_allocator.allocate_v6d(bytes, *obj_id);
  }

  virtual void putByOffset(uint64_t offset, uint64_t key, char* val,
                           int64_t seq, uint64_t version) {
    assert(false);
  }

  uint64_t getNewOffset() {
    uint64_t ret = gart::util::FAA(&header_, 1);
    assert(ret < max_items_);
    return ret;
  }

  uint64_t getOffsetHeader() const { return stable_header_; }
  void updateHeader() { stable_header_ = header_; }

  // return offset in a row and check width
  virtual uint64_t locateCol(int col_id, uint64_t width) const { return 0; }

  virtual char* getByOffset(uint64_t offset, uint64_t version) {
    assert(false);
    return nullptr;
  }

  std::vector<size_t>& get_val_lens() { return val_lens_; }
  std::vector<size_t>& get_val_offs() { return val_off_; }
  std::vector<PropertyStoreDataType>& get_val_types() { return val_type_; }

  virtual char* getByOffset(uint64_t offset, int columnID, uint64_t version,
                            uint64_t* walk_cnt = nullptr) = 0;

  // clean the pages whose version < `version`
  virtual void gc(uint64_t version) {}

  const std::vector<gart::VPropMeta>& get_blob_metas() const {
    return blob_metas_;
  }

  // schema
  std::vector<size_t> val_lens_;
  std::vector<size_t> val_off_;
  std::vector<PropertyStoreDataType> val_type_;

  class RowCursor {
   public:
    virtual void seekOffset(uint64_t begin, uint64_t end) = 0;
    virtual bool nextRow() = 0;
    virtual uint64_t key() const = 0;
    virtual char* value() const = 0;
  };

  class ColCursor {
   public:
    virtual void seekOffset(uint64_t begin, uint64_t end) = 0;
    virtual bool nextRow(uint64_t* walk_cnt = nullptr) = 0;
    virtual char* value() const = 0;

    virtual uint64_t cur() const = 0;  // current offset
    virtual char* base() const = 0;    // base ptr of static columns
  };

  // col_ids: movable columns
  virtual char* col(int col_id, uint64_t* len = nullptr) const {
    assert(false);
    return nullptr;
  }

  // virtual const std::vector<uint64_t> &getKeyCol() const {
  //   assert(false);
  //   return *(new std::vector<uint64_t>);  // unreachable
  // }

  virtual std::unique_ptr<RowCursor> getRowCursor(uint64_t ver) const {
    return nullptr;
  }

  virtual std::unique_ptr<ColCursor> getColCursor(int col_id,
                                                  uint64_t ver) const {
    return nullptr;
  }

  static inline void assign_prop(int data_type, void* prop_ptr,
                                 const std::string_view& val) {
    if (unlikely(val.size() == 0)) {
      LOG(ERROR) << "Empty value for data type: " << data_type;
    }

    try {
      switch (data_type) {
      case CHAR:
        assign(prop_ptr, val.at(0));
        break;
      case SHORT:
        assign(prop_ptr, int16_t(stoi(std::string(val))));
        break;
      case INT:
        assign(prop_ptr, stoi(std::string(val)));
        break;
      case LONG:
        assign(prop_ptr, stoll(std::string(val)));
        break;
      case FLOAT:
        assign(prop_ptr, stof(std::string(val)));
        break;
      case DOUBLE:
        assign(prop_ptr, stod(std::string(val)));
        break;
      case STRING:
        // use string id (str_offset << 16 | str_len) instead of itself
        assign(prop_ptr, stoull(std::string(val)));
        break;
      case DATE:
        assign(prop_ptr, stoi(std::string(val)));
        break;
      case DATETIME:
        assign(prop_ptr, stoll(std::string(val)));
        break;
      case TIME:
        assign(prop_ptr, stoll(std::string(val)));
        break;
      case TIMESTAMP:
        assign_inline_str<gart::graph::TimeStamp>(prop_ptr, val);
        break;
      default:
        LOG(ERROR) << "Unsupported data type: " << data_type;
      }
    } catch (std::exception& e) {
      LOG(ERROR) << "Failed to assign property: " << e.what()
                 << ", data type: " << data_type << ", value: " << val;
    }
  }

 protected:
  explicit Property(uint64_t max_items)
      : max_items_(max_items), header_(0), stable_header_(0) {}

  static inline void copy_val_(char* dst, const char* src, uint64_t sz) {
    switch (sz) {
    case 1:
      *reinterpret_cast<uint8_t*>(dst) =
          *reinterpret_cast<uint8_t*>(const_cast<char*>(src));
      break;
    case 2:
      *reinterpret_cast<uint16_t*>(dst) =
          *reinterpret_cast<uint16_t*>(const_cast<char*>(src));
      break;
    case 4:
      *reinterpret_cast<uint32_t*>(dst) =
          *reinterpret_cast<uint32_t*>(const_cast<char*>(src));
      break;
    case 8:
      *reinterpret_cast<uint64_t*>(dst) =
          *reinterpret_cast<uint64_t*>(const_cast<char*>(src));
      break;
    default:
      memcpy(dst, src, sz);
      break;
    }
  }

  // meta data about logic table
  const uint64_t max_items_;

  std::vector<gart::VPropMeta> blob_metas_;

  seggraph::SparseArrayAllocator<char> array_allocator;

#if 1  // cache for performance!
  uint64_t padding_[8];
  volatile uint64_t header_;
  uint64_t padding2_[8];
  volatile uint64_t stable_header_;
  uint64_t padding3_[8];
#endif

 private:
  template <typename T>
  static inline void assign(void* ptr, T val) {
    *reinterpret_cast<T*>(ptr) = val;
  }

  template <typename T>
  static inline void assign_inline_str(void* ptr, const std::string_view& val) {
    reinterpret_cast<T*>(ptr)->assign(val);
  }
};  // NOLINT(readability/braces)

}  // namespace property
}  // namespace gart

#endif  // VEGITO_SRC_PROPERTY_PROPERTY_H_
