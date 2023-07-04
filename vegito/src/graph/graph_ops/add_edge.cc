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
using SegGraph = seggraph::SegGraph;
using vertex_t = seggraph::vertex_t;

void process_add_edge(std::vector<std::string> cmd,
                      graph::GraphStore* graph_store) {
  int write_epoch = 0, write_seq = 0;
  write_epoch = stoi(cmd[0]);
  int elabel = stoi(cmd[1]);
  uint64_t src_vid = static_cast<uint64_t>(stoll(cmd[2]));
  uint64_t dst_vid = static_cast<uint64_t>(stoll(cmd[3]));
  gart::IdParser<seggraph::vertex_t> parser;
  parser.Init(graph_store->get_total_partitions(),
              graph_store->get_total_vertex_label_num());
  auto max_outer_id_offset =
      (((vertex_t) 1) << parser.GetOffsetWidth()) - (seggraph::vertex_t) 1;
  auto src_fid = parser.GetFid(src_vid);
  auto dst_fid = parser.GetFid(dst_vid);
  if (src_fid != graph_store->get_local_pid() &&
      dst_fid != graph_store->get_local_pid()) {
    return;
  }
  auto src_label = parser.GetLabelId(src_vid);
  auto dst_label = parser.GetLabelId(dst_vid);

  seggraph::SegGraph* src_graph =
      graph_store->get_graph<seggraph::SegGraph>(src_label);
  seggraph::SegGraph* dst_graph =
      graph_store->get_graph<seggraph::SegGraph>(dst_label);
  // this `auto` is necessary, the p_write may Transaction or EpochGraphWriter
  auto src_writer = src_graph->create_graph_writer(write_epoch);  // write epoch
  auto dst_writer = dst_graph->create_graph_writer(write_epoch);  // write epoch

  // process edge prop
  uint64_t edge_prop_bytes = graph_store->get_edge_prop_total_bytes(
      elabel + graph_store->get_total_vertex_label_num());
  char* prop_buffer = reinterpret_cast<char*>(malloc(edge_prop_bytes));
  for (auto idx = 4; idx < cmd.size(); idx++) {
    auto dtype = graph_store->get_edge_property_dtypes(
        elabel + graph_store->get_total_vertex_label_num(), idx - 4);
    uint64_t property_offset = graph_store->get_edge_prop_prefix_bytes(
        elabel + graph_store->get_total_vertex_label_num(), idx - 4);
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
  std::string buf(prop_buffer, edge_prop_bytes);
  std::string_view edge_data(buf);
  free(prop_buffer);

  if (src_fid == graph_store->get_local_pid() &&
      dst_fid != graph_store->get_local_pid()) {
    seggraph::SegGraph* ov_graph = graph_store->get_ov_graph(dst_label);
    auto ov_writer = ov_graph->create_graph_writer(write_epoch);
    uint64_t ov = graph_store->get_lid(dst_label, dst_vid);
    if (ov == uint64_t(-1)) {
      // we need to add dst_vertex to outer vertices
      ov = ov_writer.new_vertex();
      graph_store->set_lid(dst_label, dst_vid, ov);
      auto dst_lid = parser.GenerateId(0, dst_label, max_outer_id_offset - ov);
      std::shared_ptr<hashmap_t> hmap;
      graph_store->set_ovg2l(hmap, dst_label, dst_vid, dst_lid);
      graph_store->add_outer(dst_label, dst_lid);
      graph_store->set_ovl2g(dst_label, ov, dst_vid);
    }
    auto src_offset = parser.GetOffset(src_vid);
    auto src_lid = parser.GenerateId(0, src_label, src_offset);
    auto dst_lid = parser.GenerateId(0, dst_label, max_outer_id_offset - ov);
    src_writer.put_edge(src_offset, elabel, seggraph::EOUT, dst_lid, edge_data);
    ov_writer.put_edge(ov, elabel, seggraph::EIN, src_lid, edge_data);
  } else if (src_fid != graph_store->get_local_pid() &&
             dst_fid == graph_store->get_local_pid()) {
    SegGraph* ov_graph = graph_store->get_ov_graph(src_label);
    auto ov_writer = ov_graph->create_graph_writer(write_epoch);
    uint64_t ov = graph_store->get_lid(src_label, src_vid);
    if (ov == uint64_t(-1)) {
      ov = ov_writer.new_vertex();
      graph_store->set_lid(src_label, src_vid, ov);
      auto src_lid = parser.GenerateId(0, src_label, max_outer_id_offset - ov);
      std::shared_ptr<hashmap_t> hmap;
      graph_store->set_ovg2l(hmap, src_label, src_vid, src_lid);
      graph_store->add_outer(src_label, src_lid);
      graph_store->set_ovl2g(src_label, ov, src_vid);
    }
    auto dst_offset = parser.GetOffset(dst_vid);
    auto src_lid = parser.GenerateId(0, src_label, max_outer_id_offset - ov);
    auto dst_lid = parser.GenerateId(0, dst_label, dst_offset);
    ov_writer.put_edge(ov, elabel, seggraph::EOUT, dst_lid, edge_data);
    dst_writer.put_edge(dst_offset, elabel, seggraph::EIN, src_lid, edge_data);
  } else {
    auto src_offset = parser.GetOffset(src_vid);
    auto dst_offset = parser.GetOffset(dst_vid);
    vertex_t src_lid = parser.GenerateId(0, src_label, src_offset);
    vertex_t dst_lid = parser.GenerateId(0, dst_label, dst_offset);

    // inner edges
    src_writer.put_edge(src_offset, elabel, seggraph::EOUT, dst_lid, edge_data);
    dst_writer.put_edge(dst_offset, elabel, seggraph::EIN, src_lid, edge_data);
  }
}

}  // namespace graph
}  // namespace gart
