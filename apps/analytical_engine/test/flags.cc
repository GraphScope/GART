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

#include "flags.h"

#include <gflags/gflags.h>

DEFINE_string(etcd_endpoint, "http://127.0.0.1:2379",
              "etcd endpoint to get graph schema.");
DEFINE_int32(read_epoch, 0, "read epoch for graph reader.");
DEFINE_string(meta_prefix, "gart_meta_", "meta prefix for etcd.");
DEFINE_string(app_name, "sssp", "gart app name.");
DEFINE_string(sssp_source_label, "", "source label id for sssp.");
DEFINE_int32(sssp_source_oid, 0, "source oid for sssp.");
DEFINE_string(sssp_weight_name, "", "weight name for sssp.");
