/*
 *
 * The file seggraph/block_manager.hpp is referred and derived from project
 * livegraph,
 *
 *    https://github.com/thu-pacman/LiveGraph
 *
 * which has the following license:
 *
 * Copyright 2020 Guanyu Feng, Tsinghua University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <iostream>
#include <vector>

#include "common/util/likely.h"
#include "glog/logging.h"
#include "tbb/enumerable_thread_specific.h"

#include "seggraph/types.hpp"

namespace seggraph {
class BlockManager {
 public:
  constexpr static uintptr_t NULLPOINTER = 0;  // UINTPTR_MAX;

  static uint64_t allocated_mem_size;

  explicit BlockManager(int _vlabel, size_t _capacity)
      : vlabel(_vlabel),
        capacity(_capacity),
        mutex(),
        used_size(0),
        fd(EMPTY_FD),
        file_size(FILE_TRUNC_SIZE),
        data(nullptr),
        enough(false),
        free_blocks(std::vector<std::vector<uintptr_t>>(
            LARGE_BLOCK_THRESHOLD, std::vector<uintptr_t>())),
        large_free_blocks(MAX_ORDER, std::vector<uintptr_t>()) {
    null_holder = alloc(LARGE_BLOCK_THRESHOLD);
  }

  ~BlockManager() {
    free(null_holder, LARGE_BLOCK_THRESHOLD);
    if (fd != EMPTY_FD)
      close(fd);
  }

  void print_free_blocks_info() {
    std::cout << "Free blocks info: " << std::endl;
    for (int i = 0; i < LARGE_BLOCK_THRESHOLD; i++) {
      std::cout << "  number of free blocks whose order = " << i << " : "
                << free_blocks.local()[i].size() << std::endl;
    }
    for (int i = LARGE_BLOCK_THRESHOLD; i < LARGE_BLOCK_THRESHOLD + 20; i++) {
      std::cout << "  number of free blocks whose order = " << i << " : "
                << large_free_blocks[i].size() << std::endl;
    }
  }

  size_t getUsedMemory() {
    size_t ret = used_size;
    for (int i = 0; i < LARGE_BLOCK_THRESHOLD; i++) {
      ret -= free_blocks.local()[i].size() * (1ul << i);
    }
    for (int i = LARGE_BLOCK_THRESHOLD; i < LARGE_BLOCK_THRESHOLD + 20; i++) {
      ret -= large_free_blocks[i].size() * (1ul << i);
    }
    return ret;
  }

  void init_buffer(char* ptr) { data = ptr; }

  uintptr_t alloc(order_t order) {
    uintptr_t pointer = NULLPOINTER;
    if (order < LARGE_BLOCK_THRESHOLD) {
      pointer = pop(free_blocks.local(), order);
    } else {
      std::lock_guard<std::mutex> lock(mutex);
      pointer = pop(large_free_blocks, order);
    }

    if (pointer == NULLPOINTER) {
      size_t block_size = 1ul << order;
      pointer = used_size.fetch_add(block_size);

      if (unlikely(getUsedMemory() > capacity)) {
        if (!enough) {
          LOG(ERROR) << "BlockManager: out of memory."
                     << " VertexLabel: " << vlabel << " Capacity: " << capacity
                     << " Used: " << getUsedMemory()
                     << " Order: " << int(order);
          enough = true;
        }
        return NULLPOINTER;
      }

      if (pointer + block_size >= file_size) {
        auto new_file_size =
            ((pointer + block_size) / FILE_TRUNC_SIZE + 1) * FILE_TRUNC_SIZE;
        std::lock_guard<std::mutex> lock(mutex);
        if (new_file_size >= file_size) {
          if (fd != EMPTY_FD) {
            if (ftruncate(fd, new_file_size) != 0)
              throw std::runtime_error("ftruncate block file error.");
          }
          file_size = new_file_size;
        }
      }
    }

    return pointer;
  }

  void free(uintptr_t block, order_t order) {
    if (order < LARGE_BLOCK_THRESHOLD) {
      push(free_blocks.local(), order, block);
    } else {
      std::lock_guard<std::mutex> lock(mutex);
      push(large_free_blocks, order, block);
    }
  }

  template <typename T>
  inline T* convert(uintptr_t block) const {
    if (__builtin_expect((block == NULLPOINTER), 0))
      return nullptr;
    return reinterpret_cast<T*>(reinterpret_cast<char*>(data) + block);
  }

  inline uintptr_t revert(uintptr_t block) const {
    return block - (uintptr_t) data;
  }

  inline uintptr_t revert(void* ptr) const {
    return reinterpret_cast<char*>(ptr) - reinterpret_cast<char*>(data);
  }

 private:
  const int vlabel;
  const size_t capacity;
  int fd;
  void* data;
  bool enough;
  std::mutex mutex;
  tbb::enumerable_thread_specific<std::vector<std::vector<uintptr_t>>>
      free_blocks;
  std::vector<std::vector<uintptr_t>> large_free_blocks;
  std::atomic<size_t> used_size, file_size;
  uintptr_t null_holder;

  uintptr_t pop(std::vector<std::vector<uintptr_t>>& free_block,
                order_t order) {
    uintptr_t pointer = NULLPOINTER;
    if (free_block[order].size()) {
      pointer = free_block[order].back();
      free_block[order].pop_back();
    }
    return pointer;
  }

  void push(std::vector<std::vector<uintptr_t>>& free_block, order_t order,
            uintptr_t pointer) {
    free_block[order].push_back(pointer);
  }

  constexpr static int EMPTY_FD = -1;
  constexpr static order_t MAX_ORDER = 64;
  constexpr static order_t LARGE_BLOCK_THRESHOLD = 20;
  constexpr static size_t FILE_TRUNC_SIZE = 1ul << 30;  // 1GB
};

class BlockManagerLibc {
 public:
  constexpr static uintptr_t NULLPOINTER = UINTPTR_MAX;

  uintptr_t alloc(order_t order) {
    auto p = aligned_alloc(1ul << order, 1ul << order);
    if (!p)
      throw std::runtime_error("Failed to alloc block");
    return reinterpret_cast<std::uintptr_t>(p);
  }

  void free(uintptr_t block, order_t order) {
    ::free(reinterpret_cast<void*>(block));
  }

  template <typename T>
  T* convert(uintptr_t block) {
    if (block == NULLPOINTER)
      return nullptr;
    return reinterpret_cast<T*>(block);
  }
};
}  // namespace seggraph
