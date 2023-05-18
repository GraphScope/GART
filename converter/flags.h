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

#ifndef CONVERTER_FLAGS_H_
#define CONVERTER_FLAGS_H_

#include "gflags/gflags_declare.h"

DECLARE_string(read_kafka_broker_list);
DECLARE_string(write_kafka_broker_list);
DECLARE_string(read_kafka_topic);
DECLARE_string(write_kafka_topic);

DECLARE_int32(logs_per_epoch);

DECLARE_string(rg_mapping_file_path);

DECLARE_int32(numbers_of_subgraphs);

#endif  // CONVERTER_FLAGS_H_
