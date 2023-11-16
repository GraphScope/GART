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

#ifndef VEGITO_INCLUDE_UTIL_ALLOCATOR_HPP_
#define VEGITO_INCLUDE_UTIL_ALLOCATOR_HPP_

#include <sys/mman.h>

#include <limits>
#include <memory>
#include <string>

#include "glog/logging.h"
#include "vineyard/client/client.h"
#include "vineyard/client/ds/blob.h"

#include "framework/config.h"  // NOLINT(build/include_subdir)

struct SparseArrayAllocator {
  SparseArrayAllocator() : client_(new vineyard::Client), init_client_(true) {
    std::string ipc_socket = gart::framework::config.getIPCScoket();
    VINEYARD_CHECK_OK(client_->Connect(ipc_socket));

    VINEYARD_CHECK_OK(client_->InstanceStatus(v6d_status_));
  }

  explicit SparseArrayAllocator(vineyard::Client* client)
      : client_(client), init_client_(false) {
    VINEYARD_CHECK_OK(client_->InstanceStatus(v6d_status_));
  }

  ~SparseArrayAllocator() {
    if (init_client_) {
      assert(client_);
      client_->Disconnect();
      delete client_;
      client_ = nullptr;
    }
  }

  vineyard::Client* get_client() { return client_; }

  void v6d_usage_limit(size_t& usage, size_t& limit) const {
    usage = v6d_status_->memory_usage;
    limit = v6d_status_->memory_limit;
  }

  template <typename T = char>
  T* allocate_v6d(size_t n, vineyard::ObjectID& oid) {
    size_t size = n * sizeof(T);
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw std::bad_alloc();

    size_t usage = v6d_status_->memory_usage;
    size_t limit = v6d_status_->memory_limit;
    // printf("Vineyard memory usage %.2f + %.2f = %.2f MB\n",
    //        usage / 1.0 / (1 << 20), size / 1.0 / (1 << 20),
    //        (usage + size) / 1.0 / (1 << 20));
    if (usage + size > limit) {
      LOG(ERROR) << "Vineyard memory usage " << usage << " + " << size
                 << " exceeds limit " << limit;
      assert(false);
      exit(-1);
      return nullptr;
    }

    std::unique_ptr<vineyard::BlobWriter> blob_writer;
    std::shared_ptr<vineyard::Blob> blob;
    VINEYARD_CHECK_OK(client_->CreateBlob(size, blob_writer));
    VINEYARD_CHECK_OK(client_->GetBlob(blob_writer->id(), true, blob));
    auto data = reinterpret_cast<void*>(blob_writer->data());
    oid = blob_writer->id();
    return static_cast<T*>(data);
  }

  void deallocate_v6d(vineyard::ObjectID oid) {
    VINEYARD_CHECK_OK(client_->DelData(oid));
  }

  template <typename T = char>
  T* allocate(size_t n) {
    size_t size = n * sizeof(T);
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw std::bad_alloc();
    auto data = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (data == MAP_FAILED)
      throw std::bad_alloc();
    return static_cast<T*>(data);
  }

  template <typename T = char>
  void deallocate(T* data, size_t n) noexcept {
    size_t size = n * sizeof(T);
    munmap(data, size);
  }

 private:
  const bool init_client_;
  vineyard::Client* client_;

  std::shared_ptr<struct vineyard::InstanceStatus> v6d_status_;
};

#endif  // VEGITO_INCLUDE_UTIL_ALLOCATOR_HPP_
