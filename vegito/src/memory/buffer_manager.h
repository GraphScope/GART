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

#ifndef VEGITO_SRC_MEMORY_BUFFER_MANAGER_H_
#define VEGITO_SRC_MEMORY_BUFFER_MANAGER_H_

#include <cstdint>

#include "util/allocator.hpp"
#include "vineyard/client/ds/blob.h"

namespace gart {
namespace memory {

// Buffer manager for the memory pool on Vineyard
// TODO(ssj): Now only single-threaded access is supported.
class BufferManager {
 public:
  explicit BufferManager(uint64_t capacity);
  BufferManager(uint64_t capacity, vineyard::Client* v6d_client);
  ~BufferManager();

 private:
  void init_();

  const uint64_t capacity_;
  SparseArrayAllocator<void> array_allocator_;

  uint64_t size_;  // allocated size
  vineyard::ObjectID buffer_oid_;
  char* buffer_;
};

}  // namespace memory
}  // namespace gart

#endif  // VEGITO_SRC_MEMORY_BUFFER_MANAGER_H_
