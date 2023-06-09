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

namespace {

template <typename T>
inline void assign(void* ptr, T val) {
  *reinterpret_cast<T*>(ptr) = val;
}

}  // namespace

namespace gart {
namespace graph {

void assign_prop(int data_type, void* prop_ptr, const std::string& val) {
  try {
    switch (data_type) {
    case CHAR:
      assign(prop_ptr, val.at(0));
      break;
    case SHORT:
      assign(prop_ptr, short(std::stoi(val)));
      break;
    case INT:
      assign(prop_ptr, std::stoi(val));
      break;
    case LONG:
      assign(prop_ptr, std::stoll(val));
      break;
    case FLOAT:
      assign(prop_ptr, std::stof(val));
      break;
    case DOUBLE:
      assign(prop_ptr, std::stod(val));
      break;
    case STRING:
      assign(prop_ptr, ldbc::String(val));
      break;
    case TEXT:
      assign(prop_ptr, ldbc::Text(val));
      break;
    case DATE:
      assign(prop_ptr, ldbc::Date(val));
      break;
    case DATETIME:
      assign(prop_ptr, ldbc::DateTime(val));
      break;
    case LONGSTRING:
      assign(prop_ptr, ldbc::LongString(val));
      break;
    default:
      LOG(ERROR) << "Unsupported data type: " << data_type;
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "Failed to assign property: " << e.what()
               << ", data type: " << data_type << ", value: " << val;
  }
}

void process_add_vertex(const std::vector<std::string>& cmd,
                        graph::GraphStore* graph_store) {
  int write_epoch = stoi(cmd[0]);
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
    void* prop_ptr = prop_buffer + property_offset;
    assign_prop(dtype, prop_ptr, cmd[idx]);
  }
  property->insert(v, vid, prop_buffer, write_epoch);
  free(prop_buffer);
}

}  // namespace graph
}  // namespace gart
