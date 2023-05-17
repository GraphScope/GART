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

#ifndef ANALYTICAL_ENGINE_CORE_PARALLEL_GART_PARALLEL_H_
#define ANALYTICAL_ENGINE_CORE_PARALLEL_GART_PARALLEL_H_

#include <algorithm>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "interfaces/fragment/iterator.h"

namespace gart {
template <typename ITER_FUNC_T>
inline void ForEach(gart::VertexIterator& range, const ITER_FUNC_T& iter_func,
                    int chunk_size = 10) {
  int thread_num_ = 4;
  std::vector<std::thread> threads(thread_num_);
  int vertex_num = 0;
  std::vector<vertex_t> vertices;
  while (range.valid()) {
    vertices.push_back(range.vertex());
    vertex_num++;
    range.next();
  }

  std::atomic<int> cur(0);
  int end = vertex_num;
  for (auto i = 0; i < thread_num_; ++i) {
    threads[i] = std::thread(
        [&cur, chunk_size, &iter_func, end, vertices](uint32_t tid) {
          while (true) {
            int cur_beg = std::min(cur.fetch_add(chunk_size), end);
            int cur_end = std::min(cur_beg + chunk_size, end);
            if (cur_beg == cur_end) {
              break;
            }
            for (int i = cur_beg; i < cur_end; ++i) {
              iter_func(tid, vertices[i]);
            }
          }
        },
        i);
    // setThreadAffinity(threads[i], i);
  }
  for (auto& thrd : threads) {
    thrd.join();
  }
}
}  // namespace gart

#endif  // ANALYTICAL_ENGINE_CORE_PARALLEL_GART_PARALLEL_H_
