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

#include <cassert>

#include "graph/graph_ops.h"
#include "graph/type_def.h"

using std::string;

using gart::property::StringViewList;

namespace gart {
namespace graph {

void process_update_vertex(const StringViewList& cmd,
                           graph::GraphStore* graph_store) {
  int write_epoch = stoi(string(cmd[0]));
  uint64_t vid = static_cast<uint64_t>(stoll(string(cmd[1])));
  StringViewList props(cmd.begin() + 3, cmd.end());
  graph_store->update_inner_vertex(write_epoch, vid, props);
}

}  // namespace graph
}  // namespace gart
