/** Copyright 2020-2023 Alibaba Group Holding Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INTERFACES_FRAGMENT_PROPERTY_UTIL_H_
#define INTERFACES_FRAGMENT_PROPERTY_UTIL_H_

#include "util/inline_str.h"
#include "vineyard/client/ds/blob.h"

namespace gart {

using obj_id_t = vineyard::ObjectID;

typedef int Date;
typedef int64_t DateTime;
typedef int64_t Time;
typedef inline_str_fixed<22> TimeStamp;
// format for debezium: 2010-12-30T17:15:00.0Z

struct VertexPropMeta {
  int prop_id;
  int val_size;
  bool updatable;
  obj_id_t object_id;
  uintptr_t header;  // offset for colblob header
  std::string name;
  int dtype;
};

class PageHeader {
 public:
  int get_epoch() { return ver_; }
  PageHeader* get_prev(uintptr_t base_addr) {
    return (PageHeader*) (base_addr + prev_ptr_);
  }
  char* get_data() {
    // return ((char*)this) + sizeof(*this);
    return content;
  }

 private:
  uint64_t ver_;
  uintptr_t prev_ptr_;
  // unuse for reader
  uint64_t min_ver;
  PageHeader* prev_;
  PageHeader* next;
  char content[0];
};

class FlexColBlobHeader {
 public:
  PageHeader* get_page_header_ptr(uintptr_t base_ptr, int loc) {
    return (PageHeader*) (base_ptr + page_ptr[loc]);
  }

  int get_num_row_per_page() { return num_row_per_page_; }

 private:
  int num_row_per_page_;
  uintptr_t page_ptr[0];
};

}  // namespace gart

#endif  // INTERFACES_FRAGMENT_PROPERTY_UTIL_H_
