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

#include "converter/flags.h"

DEFINE_string(read_kafka_broker_list, "127.0.0.1:9092",
              "Kafka broker list for reading TxnLogs.");
DEFINE_string(write_kafka_broker_list, "127.0.0.1:9092",
              "Kafka broker list for writing UnifiedLogs.");
DEFINE_string(read_kafka_topic, "binlog", "Kafka topic for reading TxnLogs.");
DEFINE_string(write_kafka_topic, "unified_log",
              "Kafka topic for writing UnifiedLogs.");

DEFINE_int32(logs_per_epoch, 10000, "logs_per_epoch.");
DEFINE_int32(seconds_per_epoch, 60, "seconds_per_epoch.");
DEFINE_bool(use_logs_per_epoch, true, "use_logs_per_epoch.");

DEFINE_string(etcd_endpoint, "127.0.0.1:2379", "etcd endpoint.");
DEFINE_string(etcd_prefix, "", "etcd prefix.");

DEFINE_int32(subgraph_num, 1, "Number of subgraphs.");

DEFINE_int32(checkpoint_interval, 10, "Checkpoint interval in minutes.");
DEFINE_string(checkpoint_dir, "/tmp/checkpoint", "Checkpoint directory.");

DEFINE_bool(enable_bulkload, false, "Enable bulkload from existing data.");

DEFINE_int32(num_threads, 4, "Number of threads for processing logs.");
DEFINE_int32(max_queue_size, 20480, "Max queue size for logs.");
