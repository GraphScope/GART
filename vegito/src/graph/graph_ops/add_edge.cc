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
#include "property/property.h"

// using namespace std;

using seggraph::SegGraph;
using seggraph::vertex_t;
using std::string;
using std::string_view;

using gart::property::PropertyStoreDataType;
using gart::property::StringViewList;

namespace gart {
namespace graph {

void process_add_edge(const StringViewList& cmd,
                      graph::GraphStore* graph_store) {
  int write_epoch = stoi(string(cmd[0]));
  int elabel = stoi(string(cmd[1]));
  uint64_t src_vid = static_cast<uint64_t>(stoll(string(cmd[2])));
  uint64_t dst_vid = static_cast<uint64_t>(stoll(string(cmd[3])));
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

  // process edge properties

  // remove first 6 elements (epoch, elabel, src_vid, dst_vid, src_external_id,
  // dst_external_id)
  StringViewList eprop(cmd.begin() + 6, cmd.end());
  string buf;
  graph_store->construct_eprop(elabel, eprop, buf);
  string_view edge_data(buf);

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

      uint64_t* outer_external_id_store_addr =
          graph_store->get_outer_external_id_store(dst_label);
      if (graph_store->get_external_id_dtype(dst_label) ==
          PropertyStoreDataType::STRING) {
        std::string dst_external_id = string(cmd[5]);
        uint64_t value = graph_store->put_cstring(dst_external_id);
        outer_external_id_store_addr[ov] = value;
      } else {
        int64_t dst_external_id = stoll(string(cmd[5]));
        outer_external_id_store_addr[ov] = dst_external_id;
      }
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

      uint64_t* outer_external_id_store_addr =
          graph_store->get_outer_external_id_store(src_label);
      if (graph_store->get_external_id_dtype(src_label) ==
          PropertyStoreDataType::STRING) {
        std::string src_external_id = string(cmd[4]);
        uint64_t value = graph_store->put_cstring(src_external_id);
        outer_external_id_store_addr[ov] = value;
      } else {
        int64_t src_external_id = stoll(string(cmd[4]));
        outer_external_id_store_addr[ov] = src_external_id;
      }
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
