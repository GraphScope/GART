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

#ifndef VEGITO_SRC_GRAPH_GRAPH_OPS_H_
#define VEGITO_SRC_GRAPH_GRAPH_OPS_H_

#include <string>

#include "graph/graph_store.h"
#include "graph/type_def.h"

namespace gart {
namespace graph {

void process_add_vertex(const property::StringViewList& cmd,
                        graph::GraphStore* graph_store);
void process_add_edge(const property::StringViewList& cmd,
                      graph::GraphStore* graph_store);
void process_del_vertex(const property::StringViewList& cmd,
                        graph::GraphStore* graph_store);
void process_del_edge(const property::StringViewList& cmd,
                      graph::GraphStore* graph_store);
void process_update_vertex(const property::StringViewList& cmd,
                           graph::GraphStore* graph_store);
}  // namespace graph
}  // namespace gart

#endif  // VEGITO_SRC_GRAPH_GRAPH_OPS_H_
