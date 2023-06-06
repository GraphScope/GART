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

#include <gflags/gflags.h>

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

DEFINE_string(schema_file_path, "schema/rgmapping-ldbc.json",
              "user provided schema file for input.");
DEFINE_string(table_schema_file_path, "schema/db_schema.json",
              "table schema path.");

DEFINE_int32(server_num, 2, "total server number.");
DEFINE_int32(server_id, 0, "server id.");
