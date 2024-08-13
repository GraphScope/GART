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

#ifndef VEGITO_SRC_FRAMEWORK_BENCH_RUNNER_H_
#define VEGITO_SRC_FRAMEWORK_BENCH_RUNNER_H_

#include <shared_mutex>
#include <vector>

#ifdef USE_MULTI_THREADS
#include <tbb/concurrent_queue.h>
#include <thread>
#endif

#include "graph/ddl.h"
#include "graph/graph_store.h"
#include "util/status.h"

namespace gart {
namespace framework {

/* Bench runner is used to bootstrap system */
class Runner {
 public:
  Runner() {}

  void run();

 protected:
  // for graph
  std::vector<graph::GraphStore*> graph_stores_;
  std::vector<graph::RGMapping*> rg_maps_;

#ifdef USE_MULTI_THREADS
  tbb::concurrent_queue<std::string> logs_;
  bool parallel_state_ = true;
  std::vector<bool> is_working_;
  std::vector<std::shared_ptr<std::shared_timed_mutex>> working_state_mutex_;
#endif

  uint64_t latest_epoch_ = 0;
  std::chrono::high_resolution_clock::time_point start_time_;

 private:
  void load_graph_partitions_(int mac_id, int total_partitions);
  void load_graph_partitions_from_logs_(int mac_id, int total_partitions);
  void apply_log_to_store_(const std::string_view& log, int p_id);
  Status start_kafka_to_process_(int p_id);
  void start_file_stream_to_process_(int p_id);
#ifdef USE_MULTI_THREADS
  void process_log_thread(int p_id, int thread_id);
#endif
};

}  // namespace framework
}  // namespace gart

#endif  // VEGITO_SRC_FRAMEWORK_BENCH_RUNNER_H_
