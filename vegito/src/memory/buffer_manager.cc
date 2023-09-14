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
#include <string_view>

#include "glog/logging.h"

namespace gart {
namespace memory {

BufferManager::BufferManager(uint64_t capacity)
    : capacity_(capacity), size_(0), inited_(false) {
  init_();
}

BufferManager::BufferManager(uint64_t capacity, vineyard::Client* v6d_client)
    : capacity_(capacity),
      array_allocator_(v6d_client),
      size_(0),
      inited_(false) {
  init_();
}

BufferManager::BufferManager(vineyard::Client* v6d_client)
    : capacity_(0), array_allocator_(v6d_client), size_(0) {}

BufferManager::~BufferManager() {
  if (inited_) {
    auto alloc =
        std::allocator_traits<decltype(array_allocator_)>::rebind_alloc<char>(
            array_allocator_);
    alloc.deallocate_v6d(buffer_oid_);
  }
}

void BufferManager::init_() {
  auto alloc =
      std::allocator_traits<decltype(array_allocator_)>::rebind_alloc<char>(
          array_allocator_);
  vineyard::ObjectID object_id;
  buffer_ = alloc.allocate_v6d(capacity_, object_id);
  buffer_oid_ = object_id;
  inited_ = true;
}

void BufferManager::init_capacity(uint64_t capacity) {
  capacity_ = capacity;
  init_();
}

uint64_t BufferManager::put_cstring(const std::string& str) {
  return put_cstring(std::string_view(str));
}

uint64_t BufferManager::put_cstring(const std::string_view& sv) {
  uint64_t offset = size_;
  size_t new_size = offset + sv.length() + 1;
  if (new_size >= capacity_) {
    LOG(ERROR) << "BufferManager: buffer overflow (capacity: " << capacity_
               << ", size: " << size_ << ", new_size: " << new_size << ")";
    assert(false);
    return -1;
  }
  memcpy(buffer_ + offset, sv.data(), sv.length());
  buffer_[new_size - 1] = '\0';
  size_ = new_size;

  return offset;
}

char* BufferManager::get_region(uint64_t len) {
  uint64_t offset = size_;
  size_t new_size = offset + len;
  if (new_size >= capacity_) {
    LOG(ERROR) << "BufferManager: buffer overflow (capacity: " << capacity_
               << ", size: " << size_ << ", new_size: " << new_size << ")";
    assert(false);
    return nullptr;
  }
  size_ = new_size;

  return buffer_ + offset;
}

void BufferManager::get_string(uint64_t offset, uint64_t len,
                               std::string& output) const {
  output.assign(buffer_ + offset, len);
}

}  // namespace memory
}  // namespace gart
