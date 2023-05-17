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

#ifndef RESEARCH_GART_VEGITO_SRC_FRAMEWORK_BENCH_RUNNER_H_
#define RESEARCH_GART_VEGITO_SRC_FRAMEWORK_BENCH_RUNNER_H_

#include "graph/ddl.h"
#include "graph/graph_store.h"

namespace gart {
namespace framework {

/* Bench runner is used to bootstrap system */
class Runner {
 public:
  Runner() {}

  void run();

 protected:

  // for graph
  std::vector<graph::GraphStore *> graph_stores_;
  std::vector<graph::RGMapping *> rg_maps_;

  uint64_t latest_epoch_ = 0;

 private:
  void load_graph_partitions_(int mac_id, int total_partitions);
  void load_graph_partitions_from_logs_(int mac_id, int total_partitions);
  void apply_log_to_store_(std::string log, int p_id);
  void start_kafka_to_process_(int p_id);
  void start_file_stream_to_process_(int p_id);
};

}  // namespace framework
}  // namespace gart

#endif  // RESEARCH_GART_VEGITO_SRC_FRAMEWORK_BENCH_RUNNER_H_
