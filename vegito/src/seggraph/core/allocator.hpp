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

#pragma once

#include <vineyard/client/client.h>
#include <vineyard/client/ds/blob.h>

#include <sys/mman.h>

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "framework/config.h"  // NOLINT(build/include_subdir)

namespace seggraph {

template <typename T>
struct SparseArrayAllocator {
  using value_type = T;

  explicit SparseArrayAllocator(bool init_client = true)
      : client(init_client ? new vineyard::Client : nullptr), copied(false) {
    if (init_client) {
      std::string ipc_socket = gart::framework::config.getIPCScoket();
      VINEYARD_CHECK_OK(client->Connect(ipc_socket));
    }
  }

  template <class U>
  SparseArrayAllocator(const SparseArrayAllocator<U>& that)
      : client(that.client), copied(true) {
    assert(client);
  }

  ~SparseArrayAllocator() {
    if (!copied) {
      client->Disconnect();
      delete client;
      client = nullptr;
    }
  }

  void set_client(vineyard::Client* _client) { client = _client; }

  vineyard::Client* get_client() { return client; }

  T* allocate_v6d(size_t n, vineyard::ObjectID& oid) {
    size_t size = n * sizeof(T);
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw std::bad_alloc();

    std::unique_ptr<vineyard::BlobWriter> blob_writer;
    std::shared_ptr<vineyard::Blob> blob;
    VINEYARD_CHECK_OK(client->CreateBlob(size, blob_writer));
    VINEYARD_CHECK_OK(client->GetBlob(blob_writer->id(), true, blob));
    auto data = reinterpret_cast<void*>(blob_writer->data());
    oid = blob_writer->id();
    return static_cast<T*>(data);
  }

  void deallocate_v6d(vineyard::ObjectID oid) {
    VINEYARD_CHECK_OK(client->DelData(oid));
  }

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

  void deallocate(T* data, size_t n) noexcept {
    size_t size = n * sizeof(T);
    munmap(data, size);
  }

  template <class U>
  bool operator==(const SparseArrayAllocator<U>&) {
    return true;
  }
  template <class U>
  bool operator!=(const SparseArrayAllocator<U>&) {
    return false;
  }

 private:
  vineyard::Client* client;
  const bool copied;

  template <typename>
  friend struct SparseArrayAllocator;
};

}  // namespace seggraph
