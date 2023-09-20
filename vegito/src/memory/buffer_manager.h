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
#include <string>

#include "util/allocator.hpp"
#include "vineyard/client/ds/blob.h"

namespace gart {
namespace memory {

struct BufferRegion {
  BufferRegion() : ptr(nullptr), len(-1), offset(0), oid(0) {}

  char* get_ptr() const { return ptr; }
  uint64_t get_len() const { return len; }
  uint64_t get_offset() const { return offset; }
  vineyard::ObjectID get_oid() const { return oid; }

 private:
  char* ptr;
  uint64_t len;
  uint64_t offset;
  vineyard::ObjectID oid;

  friend class BufferManager;
};

// Buffer manager for the memory pool on Vineyard
// TODO(ssj): Now only single-threaded access is supported.
class BufferManager {
 public:
  // Allocate a buffer of size 'size' from the memory pool and create a vineyard
  // client
  explicit BufferManager(uint64_t capacity);

  // Use the given vineyard client, the capacity is undetermined
  // need to call init_capacity() to allocate the buffer
  explicit BufferManager(vineyard::Client* v6d_client);

  // Allocate a buffer of size 'size' from the memory pool and use the given
  // vineyard client
  BufferManager(uint64_t capacity, vineyard::Client* v6d_client);

  ~BufferManager();

  void init_capacity(uint64_t capacity);

  uint64_t get_capacity() const { return capacity_; }
  char* get_buffer() const { return buffer_; }
  vineyard::ObjectID get_buffer_oid() const { return buffer_oid_; }

  uint64_t get_size() const { return size_; }
  void set_size(uint64_t size) { size_ = size; }

  // Allocate a buffer for a string
  // Return the start offset of the string in buffer (-1 if failed)
  // `str` is the pointer to the string (C++ style)
  // `len` is the length of the string  (C++ style)
  // Set the string as the C systle string (ended by '\0')
  // TODO(ssj): Need to support thread-safety
  uint64_t put_cstring(const std::string& str);

  uint64_t put_cstring(const std::string_view& sv);

  void get_string(uint64_t offset, uint64_t len, std::string& output) const;

  // Allocate a buffer region
  // Return the start offset of the region in buffer (`nullptr` if failed)
  // `len` is the length of the region
  // `region` is the output parameter
  // TODO(ssj): Need to support thread-safety
  void get_region(uint64_t len, BufferRegion& region);

 private:
  void init_();

  bool inited_;
  uint64_t capacity_;
  SparseArrayAllocator array_allocator_;

  uint64_t size_;  // allocated size
  vineyard::ObjectID buffer_oid_;
  char* buffer_;
};

}  // namespace memory
}  // namespace gart

#endif  // VEGITO_SRC_MEMORY_BUFFER_MANAGER_H_
