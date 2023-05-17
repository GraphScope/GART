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

#include "config.h"        // NOLINT(build/include_subdir)
#include "system_flags.h"  // NOLINT(build/include_subdir)

namespace gart {
namespace framework {

Config config;

void Config::parse_sys_args(int argc, char **argv) {
  ipc_socket_ = FLAGS_v6d_ipc_socket;
  num_servers_ = FLAGS_server_num;
  server_id_ = FLAGS_server_id;
}

void Config::printConfig() const {}

}  // namespace framework
}  // namespace gart
