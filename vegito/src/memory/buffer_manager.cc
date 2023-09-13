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

#include "memory/buffer_manager.h"

namespace gart {
namespace memory {

BufferManager::BufferManager(uint64_t capacity)
    : capacity_(capacity), size_(0) {
  init_();
}

BufferManager::BufferManager(uint64_t capacity, vineyard::Client* v6d_client)
    : capacity_(capacity), array_allocator_(v6d_client), size_(0) {
  init_();
}

void BufferManager::init_() {
  auto alloc =
      std::allocator_traits<decltype(array_allocator_)>::rebind_alloc<char>(
          array_allocator_);
  vineyard::ObjectID object_id;
  buffer_ = alloc.allocate_v6d(capacity_, object_id);
  buffer_oid_ = object_id;
}

}  // namespace memory
}  // namespace gart
