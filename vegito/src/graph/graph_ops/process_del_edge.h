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

#ifndef RESEARCH_GART_VEGITO_SRC_GRAPH_GRAPH_OPS_PROCESS_DEL_EDGE_H_
#define RESEARCH_GART_VEGITO_SRC_GRAPH_GRAPH_OPS_PROCESS_DEL_EDGE_H_

#include "graph/type_def.h"
#include "graph/graph_store.h"

namespace gart {
namespace graph {
using SegGraph = seggraph::SegGraph;
using vertex_t = seggraph::vertex_t;
void process_del_edge(std::vector<std::string> cmd,
                      graph::GraphStore *graph_store) {
  int write_epoch = 0, write_seq = 0;
  write_epoch = stoi(cmd[0]);
  int elabel = stoi(cmd[1]);
  uint64_t src_vid = static_cast<uint64_t>(stoll(cmd[2]));
  uint64_t dst_vid = static_cast<uint64_t>(stoll(cmd[3]));
  gart::IdParser<vertex_t> parser;
  parser.Init(graph_store->get_total_partitions(),
              graph_store->get_total_vertex_label_num());
  auto max_outer_id_offset =
      (((vertex_t)1) << parser.GetOffsetWidth()) - (vertex_t)1;

  uint64_t edge_prop_bytes = graph_store->get_edge_prop_total_bytes(
      elabel + graph_store->get_total_vertex_label_num());
  char *prop_buffer = reinterpret_cast<char *>(malloc(edge_prop_bytes));
  memset(prop_buffer, 0, edge_prop_bytes);
  std::string buf(prop_buffer, edge_prop_bytes);
  std::string_view edge_data(buf);
  free(prop_buffer);
  auto src_fid = parser.GetFid(src_vid);
  auto dst_fid = parser.GetFid(dst_vid);
  if (src_fid != graph_store->get_local_pid() &&
      dst_fid != graph_store->get_local_pid()) {
    return;
  }
  auto src_label = parser.GetLabelId(src_vid);
  auto dst_label = parser.GetLabelId(dst_vid);

  seggraph::SegGraph *src_graph;
  seggraph::SegGraph *dst_graph;
  uint64_t src_offset_reverse, dst_offset_reverse;
  uint64_t src_offset, dst_offset;  // offset in lid

  if (src_fid == graph_store->get_local_pid() &&
      dst_fid != graph_store->get_local_pid()) {
    src_offset = parser.GetOffset(src_vid);
    src_offset_reverse = src_offset;
    dst_offset_reverse = graph_store->get_lid(dst_label, dst_vid);
    assert(dst_offset_reverse != -1);
    dst_offset = max_outer_id_offset - dst_offset_reverse;
    src_graph = graph_store->get_graph<seggraph::SegGraph>(src_label);
    dst_graph = graph_store->get_ov_graph(dst_label);

  } else if (src_fid != graph_store->get_local_pid() &&
             dst_fid == graph_store->get_local_pid()) {
    src_offset_reverse = graph_store->get_lid(src_label, src_vid);
    assert(src_offset_reverse != -1);
    src_offset = max_outer_id_offset - src_offset_reverse;
    dst_offset = parser.GetOffset(dst_vid);
    dst_offset_reverse = dst_offset;
    src_graph = graph_store->get_ov_graph(src_label);
    dst_graph = graph_store->get_graph<seggraph::SegGraph>(dst_label);

  } else {
    src_offset = parser.GetOffset(src_vid);
    src_offset_reverse = src_offset;
    dst_offset = parser.GetOffset(dst_vid);
    dst_offset_reverse = dst_offset;
    src_graph = graph_store->get_graph<seggraph::SegGraph>(src_label);
    dst_graph = graph_store->get_graph<seggraph::SegGraph>(dst_label);
  }

  {
    auto src_writer =
        src_graph->create_graph_writer(write_epoch);  // write epoch
    auto dst_writer = dst_graph->create_graph_writer(write_epoch);

    seggraph::segid_t src_segid =
        src_graph->get_vertex_seg_id(src_offset_reverse);
    uint32_t src_segidx = src_graph->get_vertex_seg_idx(src_offset_reverse);
    seggraph::VegitoSegmentHeader *src_segment =
        src_writer.locate_segment(src_segid, elabel, seggraph::EOUT);
    assert(src_segment != nullptr);

    uintptr_t src_edge_block_pointer = src_segment->get_region_ptr(src_segidx);
    seggraph::VegitoEdgeBlockHeader *src_edge_block =
        src_graph->get_block_manager().convert<seggraph::VegitoEdgeBlockHeader>(
            src_edge_block_pointer);
    assert(src_edge_block != nullptr);
    size_t src_num_entries = src_edge_block->get_num_entries();
    seggraph::VegitoEdgeEntry *src_entries = src_edge_block->get_entries();
    seggraph::VegitoEdgeEntry *src_entries_cursor =
        src_entries - src_num_entries;

    std::vector<uint64_t> src_prefix_sum;
    seggraph::VegitoEdgeBlockHeader *src_cur_header = src_edge_block;
    while (src_cur_header) {
      src_prefix_sum.push_back(src_cur_header->get_num_entries());

      src_cur_header = src_graph->get_block_manager()
                           .convert<seggraph::VegitoEdgeBlockHeader>(
                               src_cur_header->get_prev_pointer());
    }

    src_prefix_sum[0] = 0;
    for (auto i = 0; i < src_prefix_sum.size() - 1; i++) {
      src_prefix_sum[i] = 0;
      for (auto j = i + 1; j < src_prefix_sum.size(); j++) {
        src_prefix_sum[i] += src_prefix_sum[j];
      }
    }
    src_prefix_sum[src_prefix_sum.size() - 1] = 0;

    int src_segment_idx = 0;
    bool is_founded = false;

    while (true) {
      while (src_entries_cursor != src_entries) {
        seggraph::vertex_t vid = src_entries_cursor->get_dst();

        auto delete_flag = vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
        if (parser.GetOffset(vid) == dst_offset && delete_flag != 1) {
          is_founded = true;
          auto del_loc = src_entries - src_entries_cursor - 1 +
                         src_prefix_sum[src_segment_idx];

          auto mask = ((seggraph::vertex_t)1)
                      << (sizeof(seggraph::vertex_t) * 8 - 1);
          del_loc = del_loc | mask;
          src_writer.put_edge(src_offset_reverse, elabel, seggraph::EOUT,
                              del_loc, edge_data);
          break;
        }
        src_entries_cursor++;
      }
      if (is_founded == true) {
        break;
      }
      src_edge_block = src_graph->get_block_manager()
                           .convert<seggraph::VegitoEdgeBlockHeader>(
                               src_edge_block->get_prev_pointer());
      if (!src_edge_block) {
        break;
      } else {
        src_entries = src_edge_block->get_entries();
        auto src_num_entries = src_edge_block->get_num_entries();
        src_entries_cursor = src_entries - src_num_entries;  // at the begining
        src_segment_idx++;
      }
    }
    if (is_founded == false) {
      LOG(ERROR) << "delete edge error";
    }

    is_founded = false;
    // process dst vertex
    seggraph::segid_t dst_segid =
        dst_graph->get_vertex_seg_id(dst_offset_reverse);
    uint32_t dst_segidx = dst_graph->get_vertex_seg_idx(dst_offset_reverse);
    seggraph::VegitoSegmentHeader *dst_segment =
        dst_writer.locate_segment(dst_segid, elabel, seggraph::EIN);
    assert(dst_segment != nullptr);

    uintptr_t dst_edge_block_pointer = dst_segment->get_region_ptr(dst_segidx);
    seggraph::VegitoEdgeBlockHeader *dst_edge_block =
        dst_graph->get_block_manager().convert<seggraph::VegitoEdgeBlockHeader>(
            dst_edge_block_pointer);
    assert(dst_edge_block != nullptr);
    size_t dst_num_entries = dst_edge_block->get_num_entries();
    seggraph::VegitoEdgeEntry *dst_entries = dst_edge_block->get_entries();
    seggraph::VegitoEdgeEntry *dst_entries_cursor =
        dst_entries - dst_num_entries;

    std::vector<uint64_t> dst_prefix_sum;
    seggraph::VegitoEdgeBlockHeader *dst_cur_header = dst_edge_block;
    while (dst_cur_header) {
      dst_prefix_sum.push_back(dst_cur_header->get_num_entries());
      dst_cur_header = dst_graph->get_block_manager()
                           .convert<seggraph::VegitoEdgeBlockHeader>(
                               dst_cur_header->get_prev_pointer());
    }

    dst_prefix_sum[0] = 0;
    for (auto i = 0; i < dst_prefix_sum.size() - 1; i++) {
      dst_prefix_sum[i] = 0;
      for (auto j = i + 1; j < dst_prefix_sum.size(); j++) {
        dst_prefix_sum[i] += dst_prefix_sum[j];
      }
    }
    dst_prefix_sum[dst_prefix_sum.size() - 1] = 0;
    int dst_segment_idx = 0;

    while (true) {
      while (dst_entries_cursor != dst_entries) {
        seggraph::vertex_t vid = dst_entries_cursor->get_dst();
        auto delete_flag = vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
        if (parser.GetOffset(vid) == src_offset && delete_flag != 1) {
          is_founded = true;
          auto del_loc = dst_entries - dst_entries_cursor - 1 +
                         dst_prefix_sum[dst_segment_idx];

          auto mask = ((seggraph::vertex_t)1)
                      << (sizeof(seggraph::vertex_t) * 8 - 1);
          del_loc = del_loc | mask;
          dst_writer.put_edge(dst_offset_reverse, elabel, seggraph::EIN,
                              del_loc, edge_data);
          break;
        }
        dst_entries_cursor++;
      }
      if (is_founded == true) {
        break;
      }
      dst_edge_block = dst_graph->get_block_manager()
                           .convert<seggraph::VegitoEdgeBlockHeader>(
                               dst_edge_block->get_prev_pointer());
      if (!dst_edge_block) {
        break;
      } else {
        dst_entries = dst_edge_block->get_entries();
        auto dst_num_entries = dst_edge_block->get_num_entries();
        dst_entries_cursor = dst_entries - dst_num_entries;  // at the begining
        dst_segment_idx++;
      }
    }
    if (is_founded == false) {
      LOG(ERROR) << "delete edge error";
    }
  }
}
}  // namespace graph
}  // namespace gart

#endif  // RESEARCH_GART_VEGITO_SRC_GRAPH_GRAPH_OPS_PROCESS_DEL_EDGE_H_
