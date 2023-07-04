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

#include "graph/graph_ops.h"

namespace gart {
namespace graph {
void process_add_vertex(std::vector<std::string> cmd,
                        graph::GraphStore* graph_store) {
  int write_epoch = 0, write_seq = 0;
  write_epoch = stoi(cmd[0]);
  uint64_t vid = static_cast<uint64_t>(stoll(cmd[1]));
  gart::IdParser<seggraph::vertex_t> parser;
  parser.Init(graph_store->get_total_partitions(),
              graph_store->get_total_vertex_label_num());

  auto fid = parser.GetFid(vid);
  if (fid != graph_store->get_local_pid()) {
    return;
  }
  auto vlabel = parser.GetLabelId(vid);
  seggraph::SegGraph* graph =
      graph_store->get_graph<seggraph::SegGraph>(vlabel);
  Property* property = graph_store->get_property(vlabel);

  auto writer = graph->create_graph_writer(write_epoch);  // write epoch

  // insert vertex
  seggraph::vertex_t v = writer.new_vertex();
  auto off = property->getNewOffset();
  assert(v == off);
  auto voffset = parser.GetOffset(vid);
  auto lid = parser.GenerateId(0, vlabel, voffset);
  graph_store->add_inner(vlabel, lid);

  // insert property
  auto prop_schema = graph_store->get_property_schema(vlabel);
  uint64_t prop_byte_size = graph_store->get_total_property_bytes(vlabel);
  char* prop_buffer = reinterpret_cast<char*>(malloc(prop_byte_size));
  for (auto idx = 2; idx < cmd.size(); idx++) {
    auto dtype = prop_schema.cols[idx - 2].vtype;
    uint64_t property_offset =
        graph_store->get_prefix_property_bytes(vlabel, idx - 2);
    if (dtype == INT) {
      *reinterpret_cast<int*>(prop_buffer + property_offset) =
          std::stoi(cmd[idx]);
    } else if (dtype == FLOAT) {
      *reinterpret_cast<float*>(prop_buffer + property_offset) =
          std::stof(cmd[idx]);
    } else if (dtype == DOUBLE) {
      *reinterpret_cast<double*>(prop_buffer + property_offset) =
          std::stod(cmd[idx]);
    } else if (dtype == LONG) {
      *reinterpret_cast<uint64_t*>(prop_buffer + property_offset) =
          std::stoll(cmd[idx]);
    } else if (dtype == CHAR) {
      *(prop_buffer + property_offset) = cmd[idx].at(0);
    } else if (dtype == STRING) {
      ldbc::String tmp(cmd[idx].c_str(), cmd[idx].length());
      *reinterpret_cast<ldbc::String*>(prop_buffer + property_offset) = tmp;
    } else if (dtype == TEXT) {
      ldbc::Text tmp(cmd[idx].c_str(), cmd[idx].length());
      *reinterpret_cast<ldbc::Text*>(prop_buffer + property_offset) = tmp;
    } else if (dtype == DATE) {
      ldbc::Date tmp(cmd[idx].c_str(), cmd[idx].length());
      *reinterpret_cast<ldbc::Date*>(prop_buffer + property_offset) = tmp;
    } else if (dtype == DATETIME) {
      ldbc::DateTime tmp(cmd[idx].c_str(), cmd[idx].length());
      *reinterpret_cast<ldbc::DateTime*>(prop_buffer + property_offset) = tmp;
    } else if (dtype == LONGSTRING) {
      ldbc::LongString tmp(cmd[idx].c_str(), cmd[idx].length());
      *reinterpret_cast<ldbc::LongString*>(prop_buffer + property_offset) = tmp;
    } else {
      assert(false);
    }
  }
  property->insert(v, vid, prop_buffer, write_seq, write_epoch);
  free(prop_buffer);
}

}  // namespace graph
}  // namespace gart
