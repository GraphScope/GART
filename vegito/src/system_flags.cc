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

#include "system_flags.h"  // NOLINT(build/include_subdir)

DEFINE_string(kafka_broker_list, "127.0.0.1:9092",
              "Kafka broker list for GART writer.");
DEFINE_string(kafka_unified_log_topic, "unified_log",
              "Kafka topic for unified logs.");
DEFINE_string(kafka_unified_log_file, "test_graph.txt",
              "The file to record unified logs.");  // for tests

DEFINE_string(etcd_endpoint, "http://127.0.0.1:2379",
              "etcd endpoint for schema.");
DEFINE_string(meta_prefix, "gart_meta_", "meta prefix for etcd.");
DEFINE_string(v6d_ipc_socket, "/var/run/vineyard.sock", "Vineyard IPC socket.");

DEFINE_int32(subgraph_num, 1, "total subgraph number.");
DEFINE_int32(subgraph_id, 0, "subgraph id.");

DEFINE_int64(default_max_vertex_number, 1 * (1ul << 26),
             "default max vertex number.");
DEFINE_int64(default_max_memory_usage_for_each_type_vertex, 10 * (1ul << 30),
             "default max memory usage for each type vertex.");  // in bytes
DEFINE_string(customized_vertex_number_memory_usage_config,
              "",  // format: "type1:100:10000,type2:100:10000"
              "customized vertex number memory usage config.");

DEFINE_int32(num_threads, 2, "number of threads");