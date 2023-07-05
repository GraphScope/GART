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
using VegitoEdgeEntry = seggraph::VegitoEdgeEntry;
using VegitoSegmentHeader = seggraph::VegitoSegmentHeader;
using VegitoEdgeBlockHeader = seggraph::VegitoEdgeBlockHeader;
using EpochBlockHeader = seggraph::EpochBlockHeader;
using segid_t = seggraph::segid_t;
using vertex_t = seggraph::vertex_t;
using SegGraph = seggraph::SegGraph;
void process_del_vertex(const std::vector<std::string>& cmd,
                        graph::GraphStore* graph_store) {
  int write_epoch = stoi(cmd[0]);
  uint64_t vid = static_cast<uint64_t>(stoll(cmd[1]));
  const int write_seq = 0;
  gart::IdParser<vertex_t> parser;
  parser.Init(graph_store->get_total_partitions(),
              graph_store->get_total_vertex_label_num());

  auto fid = parser.GetFid(vid);
  if (fid == graph_store->get_local_pid()) {  // is a inner vertex
    auto v_offset = parser.GetOffset(vid);
    auto v_label = parser.GetLabelId(vid);
    seggraph::SegGraph* src_graph =
        graph_store->get_graph<seggraph::SegGraph>(v_label);
    graph_store->delete_inner(v_label,
                              v_offset);  // delete vertex from vertex table
    src_graph->add_deleted_inner_num(1);

    // delete ralated edges
    auto src_writer =
        src_graph->create_graph_writer(write_epoch);  // write epoch
    segid_t segid = src_graph->get_vertex_seg_id(v_offset);
    uint32_t segidx = src_graph->get_vertex_seg_idx(v_offset);
    VegitoSegmentHeader* segment;
    // process outgoing edges
    for (auto elabel = 0;
         elabel < graph_store->get_schema().edge_relation.size(); elabel++) {
      segment = src_writer.locate_segment(segid, elabel, seggraph::EOUT);
      if (segment == nullptr) {
        continue;
      }
      uintptr_t edge_block_pointer = segment->get_region_ptr(segidx);
      VegitoEdgeBlockHeader* edge_block =
          src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
              edge_block_pointer);
      uintptr_t epoch_table_pointer = segment->get_epoch_table(segidx);
      EpochBlockHeader* epoch_table =
          src_graph->get_block_manager().convert<EpochBlockHeader>(
              epoch_table_pointer);
      if (!edge_block || !epoch_table) {
        continue;
      }
      size_t num_entries = edge_block->get_num_entries();
      VegitoEdgeEntry* entries = edge_block->get_entries();
      VegitoEdgeEntry* entries_cursor = entries - num_entries;
      std::vector<uint64_t> prefix_sum;
      VegitoEdgeBlockHeader* cur_header = edge_block;
      while (cur_header) {
        prefix_sum.push_back(cur_header->get_num_entries());
        cur_header =
            src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                cur_header->get_prev_pointer());
      }

      prefix_sum[0] = 0;
      for (auto i = 0; i < prefix_sum.size() - 1; i++) {
        prefix_sum[i] = 0;
        for (auto j = i + 1; j < prefix_sum.size(); j++) {
          prefix_sum[i] += prefix_sum[j];
        }
      }
      prefix_sum[prefix_sum.size() - 1] = 0;

      int segment_idx = 0;
      std::priority_queue<size_t> delete_offsets;
      std::vector<uint64_t> delete_loc;
      std::vector<seggraph::vertex_t> delete_vertices;
      // find edges
      while (true) {
        while (entries_cursor != entries) {
          seggraph::vertex_t vid = entries_cursor->get_dst();
          auto delete_flag = vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
          if (delete_flag == 1) {
            auto delete_offset_mask =
                (((seggraph::vertex_t) 1)
                 << (sizeof(seggraph::vertex_t) * 8 - 1)) -
                (seggraph::vertex_t) 1;
            auto delete_offset = vid & delete_offset_mask;
            delete_offsets.push(delete_offset);
          } else {
            if (delete_offsets.empty()) {
              delete_vertices.push_back(vid);
              delete_loc.push_back(entries - entries_cursor - 1 +
                                   prefix_sum[segment_idx]);
            } else if (delete_offsets.top() != (entries - entries_cursor - 1 +
                                                prefix_sum[segment_idx])) {
              delete_vertices.push_back(vid);
              delete_loc.push_back(entries - entries_cursor - 1 +
                                   prefix_sum[segment_idx]);
            } else {
              delete_offsets.pop();
            }
          }
          entries_cursor++;
        }
        edge_block =
            src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                edge_block->get_prev_pointer());
        if (!edge_block) {
          break;
        } else {
          entries = edge_block->get_entries();
          auto num_entries = edge_block->get_num_entries();
          entries_cursor = entries - num_entries;  // at the begining
          segment_idx++;
        }
      }

      // delete edges
      uint64_t edge_prop_bytes = graph_store->get_edge_prop_total_bytes(
          elabel + graph_store->get_total_vertex_label_num());
      char* prop_buffer = reinterpret_cast<char*>(malloc(edge_prop_bytes));
      memset(prop_buffer, 0, edge_prop_bytes);
      std::string buf(prop_buffer, edge_prop_bytes);
      std::string_view edge_data(buf);
      free(prop_buffer);
      for (auto idx = 0; idx < delete_loc.size(); idx++) {
        auto dst_offset = parser.GetOffset(delete_vertices[idx]);
        auto dst_label = parser.GetLabelId(delete_vertices[idx]);
        auto mask = ((seggraph::vertex_t) 1)
                    << (sizeof(seggraph::vertex_t) * 8 - 1);
        auto dst_loc = delete_loc[idx] | mask;

        src_writer.put_edge(v_offset, elabel, seggraph::EOUT, dst_loc,
                            edge_data);

        if (dst_offset < graph_store->get_vtable_max_inner(
                             dst_label)) {  // dst is an inner vertex
          seggraph::SegGraph* dst_graph =
              graph_store->get_graph<seggraph::SegGraph>(dst_label);
          auto dst_writer = dst_graph->create_graph_writer(write_epoch);
          segid_t dst_segid = dst_graph->get_vertex_seg_id(dst_offset);
          uint32_t dst_segidx = dst_graph->get_vertex_seg_idx(dst_offset);
          VegitoSegmentHeader* dst_segment =
              dst_writer.locate_segment(dst_segid, elabel, seggraph::EIN);
          uintptr_t dst_edge_block_pointer =
              dst_segment->get_region_ptr(dst_segidx);
          VegitoEdgeBlockHeader* dst_edge_block =
              dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                  dst_edge_block_pointer);
          std::vector<uint64_t> dst_prefix_sum;
          VegitoEdgeBlockHeader* dst_cur_header = dst_edge_block;
          while (dst_cur_header) {
            dst_prefix_sum.push_back(dst_cur_header->get_num_entries());
            dst_cur_header =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
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
          size_t dst_num_entries = dst_edge_block->get_num_entries();
          VegitoEdgeEntry* dst_entries = dst_edge_block->get_entries();
          VegitoEdgeEntry* dst_entries_cursor = dst_entries - dst_num_entries;
          bool is_founded = false;
          while (true) {
            while (dst_entries_cursor != dst_entries) {
              seggraph::vertex_t vid = dst_entries_cursor->get_dst();

              auto dst_delete_flag =
                  vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
              if (parser.GetOffset(vid) == v_offset && dst_delete_flag != 1) {
                is_founded = true;
                auto del_loc = dst_entries - dst_entries_cursor - 1 +
                               dst_prefix_sum[dst_segment_idx];
                auto mask = ((seggraph::vertex_t) 1)
                            << (sizeof(seggraph::vertex_t) * 8 - 1);

                del_loc = del_loc | mask;
                dst_writer.put_edge(dst_offset, elabel, seggraph::EIN, del_loc,
                                    edge_data);
                break;
              }
              dst_entries_cursor++;
            }
            if (is_founded) {
              break;
            }
            dst_edge_block =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                    dst_edge_block->get_prev_pointer());
            if (!dst_edge_block) {
              break;
            } else {
              dst_entries = dst_edge_block->get_entries();
              auto dst_num_entries = dst_edge_block->get_num_entries();
              dst_entries_cursor =
                  dst_entries - dst_num_entries;  // at the begining
              dst_segment_idx++;
            }
          }

        } else {  // dst is a outer vertex
          seggraph::SegGraph* dst_graph = graph_store->get_ov_graph(dst_label);
          auto dst_writer = dst_graph->create_graph_writer(write_epoch);
          auto max_outer_id_offset =
              (((vertex_t) 1) << parser.GetOffsetWidth()) - (vertex_t) 1;
          segid_t dst_segid =
              dst_graph->get_vertex_seg_id(max_outer_id_offset - dst_offset);
          uint32_t dst_segidx =
              dst_graph->get_vertex_seg_idx(max_outer_id_offset - dst_offset);
          VegitoSegmentHeader* dst_segment =
              dst_writer.locate_segment(dst_segid, elabel, seggraph::EIN);
          uintptr_t dst_edge_block_pointer =
              dst_segment->get_region_ptr(dst_segidx);
          VegitoEdgeBlockHeader* dst_edge_block =
              dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                  dst_edge_block_pointer);
          std::vector<uint64_t> dst_prefix_sum;
          VegitoEdgeBlockHeader* dst_cur_header = dst_edge_block;
          while (dst_cur_header) {
            dst_prefix_sum.push_back(dst_cur_header->get_num_entries());
            dst_cur_header =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
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

          size_t dst_num_entries = dst_edge_block->get_num_entries();
          VegitoEdgeEntry* dst_entries = dst_edge_block->get_entries();
          VegitoEdgeEntry* dst_entries_cursor = dst_entries - dst_num_entries;
          bool is_founded = false;

          while (true) {
            while (dst_entries_cursor != dst_entries) {
              seggraph::vertex_t vid = dst_entries_cursor->get_dst();

              auto dst_delete_flag =
                  vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
              if (parser.GetOffset(vid) == v_offset && dst_delete_flag != 1) {
                is_founded = true;
                auto del_loc = dst_entries - dst_entries_cursor - 1 +
                               dst_prefix_sum[dst_segment_idx];
                auto mask = ((seggraph::vertex_t) 1)
                            << (sizeof(seggraph::vertex_t) * 8 - 1);
                del_loc = del_loc | mask;
                dst_writer.put_edge(max_outer_id_offset - dst_offset, elabel,
                                    seggraph::EIN, del_loc, edge_data);
                break;
              }
              dst_entries_cursor++;
            }
            if (is_founded) {
              break;
            }
            dst_edge_block =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                    dst_edge_block->get_prev_pointer());
            if (!dst_edge_block) {
              break;
            } else {
              dst_entries = dst_edge_block->get_entries();
              auto dst_num_entries = dst_edge_block->get_num_entries();
              dst_entries_cursor =
                  dst_entries - dst_num_entries;  // at the begining
              dst_segment_idx++;
            }
          }
        }
      }
    }

    // process incoming egdes
    for (auto elabel = 0;
         elabel < graph_store->get_schema().edge_relation.size(); elabel++) {
      segment = src_writer.locate_segment(segid, elabel, seggraph::EIN);
      if (segment == nullptr) {
        continue;
      }
      uintptr_t edge_block_pointer = segment->get_region_ptr(segidx);
      VegitoEdgeBlockHeader* edge_block =
          src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
              edge_block_pointer);
      uintptr_t epoch_table_pointer = segment->get_epoch_table(segidx);
      EpochBlockHeader* epoch_table =
          src_graph->get_block_manager().convert<EpochBlockHeader>(
              epoch_table_pointer);
      if (!edge_block || !epoch_table) {
        continue;
      }
      size_t num_entries = edge_block->get_num_entries();
      VegitoEdgeEntry* entries = edge_block->get_entries();
      VegitoEdgeEntry* entries_cursor = entries - num_entries;
      std::vector<uint64_t> prefix_sum;
      VegitoEdgeBlockHeader* cur_header = edge_block;
      while (cur_header) {
        prefix_sum.push_back(cur_header->get_num_entries());
        cur_header =
            src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                cur_header->get_prev_pointer());
      }
      prefix_sum[0] = 0;
      for (auto i = 0; i < prefix_sum.size() - 1; i++) {
        prefix_sum[i] = 0;
        for (auto j = i + 1; j < prefix_sum.size(); j++) {
          prefix_sum[i] += prefix_sum[j];
        }
      }
      prefix_sum[prefix_sum.size() - 1] = 0;

      int segment_idx = 0;
      std::priority_queue<size_t> delete_offsets;
      std::vector<uint64_t> delete_loc;
      std::vector<seggraph::vertex_t> delete_vertices;
      // find edges
      while (true) {
        while (entries_cursor != entries) {
          seggraph::vertex_t vid = entries_cursor->get_dst();
          auto delete_flag = vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
          if (delete_flag == 1) {
            auto delete_offset_mask =
                (((seggraph::vertex_t) 1)
                 << (sizeof(seggraph::vertex_t) * 8 - 1)) -
                (seggraph::vertex_t) 1;
            auto delete_offset = vid & delete_offset_mask;
            delete_offsets.push(delete_offset);
          } else {
            if (delete_offsets.empty()) {
              delete_vertices.push_back(vid);
              delete_loc.push_back(entries - entries_cursor - 1 +
                                   prefix_sum[segment_idx]);
            } else if (delete_offsets.top() != (entries - entries_cursor - 1 +
                                                prefix_sum[segment_idx])) {
              delete_vertices.push_back(vid);
              delete_loc.push_back(entries - entries_cursor - 1 +
                                   prefix_sum[segment_idx]);
            } else {
              delete_offsets.pop();
            }
          }
          entries_cursor++;
        }
        edge_block =
            src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                edge_block->get_prev_pointer());
        if (!edge_block) {
          break;
        } else {
          entries = edge_block->get_entries();
          auto num_entries = edge_block->get_num_entries();
          entries_cursor = entries - num_entries;  // at the begining
          segment_idx++;
        }
      }
      // delete edges
      uint64_t edge_prop_bytes = graph_store->get_edge_prop_total_bytes(
          elabel + graph_store->get_total_vertex_label_num());
      char* prop_buffer = reinterpret_cast<char*>(malloc(edge_prop_bytes));
      memset(prop_buffer, 0, edge_prop_bytes);
      std::string buf(prop_buffer, edge_prop_bytes);
      std::string_view edge_data(buf);
      free(prop_buffer);
      for (auto idx = 0; idx < delete_loc.size(); idx++) {
        auto dst_offset = parser.GetOffset(delete_vertices[idx]);
        auto dst_label = parser.GetLabelId(delete_vertices[idx]);
        auto mask = ((seggraph::vertex_t) 1)
                    << (sizeof(seggraph::vertex_t) * 8 - 1);

        auto dst_loc = delete_loc[idx] | mask;
        src_writer.put_edge(v_offset, elabel, seggraph::EIN, dst_loc,
                            edge_data);

        if (dst_offset < graph_store->get_vtable_max_inner(dst_label)) {
          seggraph::SegGraph* dst_graph =
              graph_store->get_graph<seggraph::SegGraph>(dst_label);
          auto dst_writer = dst_graph->create_graph_writer(write_epoch);
          segid_t dst_segid = dst_graph->get_vertex_seg_id(dst_offset);
          uint32_t dst_segidx = dst_graph->get_vertex_seg_idx(dst_offset);
          VegitoSegmentHeader* dst_segment =
              dst_writer.locate_segment(dst_segid, elabel, seggraph::EOUT);
          uintptr_t dst_edge_block_pointer =
              dst_segment->get_region_ptr(dst_segidx);
          VegitoEdgeBlockHeader* dst_edge_block =
              dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                  dst_edge_block_pointer);
          std::vector<uint64_t> dst_prefix_sum;
          VegitoEdgeBlockHeader* dst_cur_header = dst_edge_block;
          while (dst_cur_header) {
            dst_prefix_sum.push_back(dst_cur_header->get_num_entries());
            dst_cur_header =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
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

          size_t dst_num_entries = dst_edge_block->get_num_entries();
          VegitoEdgeEntry* dst_entries = dst_edge_block->get_entries();
          VegitoEdgeEntry* dst_entries_cursor = dst_entries - dst_num_entries;
          bool is_founded = false;
          while (true) {
            while (dst_entries_cursor != dst_entries) {
              seggraph::vertex_t vid = dst_entries_cursor->get_dst();
              auto dst_delete_flag =
                  vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
              if (parser.GetOffset(vid) == v_offset && dst_delete_flag != 1) {
                is_founded = true;
                auto del_loc = dst_entries - dst_entries_cursor - 1 +
                               dst_prefix_sum[dst_segment_idx];
                auto mask = ((seggraph::vertex_t) 1)
                            << (sizeof(seggraph::vertex_t) * 8 - 1);
                del_loc = del_loc | mask;
                dst_writer.put_edge(dst_offset, elabel, seggraph::EOUT, del_loc,
                                    edge_data);
                break;
              }
              dst_entries_cursor++;
            }
            if (is_founded) {
              break;
            }
            dst_edge_block =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                    dst_edge_block->get_prev_pointer());
            if (!dst_edge_block) {
              break;
            } else {
              dst_entries = dst_edge_block->get_entries();
              auto dst_num_entries = dst_edge_block->get_num_entries();
              dst_entries_cursor =
                  dst_entries - dst_num_entries;  // at the begining
              dst_segment_idx++;
            }
          }

        } else {
          // dst is a outer vertex
          seggraph::SegGraph* dst_graph = graph_store->get_ov_graph(dst_label);
          auto dst_writer = dst_graph->create_graph_writer(write_epoch);
          auto max_outer_id_offset =
              (((vertex_t) 1) << parser.GetOffsetWidth()) - (vertex_t) 1;
          segid_t dst_segid =
              dst_graph->get_vertex_seg_id(max_outer_id_offset - dst_offset);
          uint32_t dst_segidx =
              dst_graph->get_vertex_seg_idx(max_outer_id_offset - dst_offset);
          VegitoSegmentHeader* dst_segment =
              dst_writer.locate_segment(dst_segid, elabel, seggraph::EOUT);
          assert(dst_segment != nullptr);
          uintptr_t dst_edge_block_pointer =
              dst_segment->get_region_ptr(dst_segidx);
          VegitoEdgeBlockHeader* dst_edge_block =
              dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                  dst_edge_block_pointer);
          std::vector<uint64_t> dst_prefix_sum;
          VegitoEdgeBlockHeader* dst_cur_header = dst_edge_block;
          while (dst_cur_header) {
            dst_prefix_sum.push_back(dst_cur_header->get_num_entries());
            dst_cur_header =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
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

          size_t dst_num_entries = dst_edge_block->get_num_entries();
          VegitoEdgeEntry* dst_entries = dst_edge_block->get_entries();
          VegitoEdgeEntry* dst_entries_cursor = dst_entries - dst_num_entries;
          bool is_founded = false;
          while (true) {
            while (dst_entries_cursor != dst_entries) {
              seggraph::vertex_t vid = dst_entries_cursor->get_dst();
              auto dst_delete_flag =
                  vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
              if (parser.GetOffset(vid) == v_offset && dst_delete_flag != 1) {
                is_founded = true;
                auto del_loc = dst_entries - dst_entries_cursor - 1 +
                               dst_prefix_sum[dst_segment_idx];
                auto mask = ((seggraph::vertex_t) 1)
                            << (sizeof(seggraph::vertex_t) * 8 - 1);
                del_loc = del_loc | mask;
                dst_writer.put_edge(max_outer_id_offset - dst_offset, elabel,
                                    seggraph::EOUT, del_loc, edge_data);
                break;
              }
              dst_entries_cursor++;
            }
            if (is_founded) {
              break;
            }
            dst_edge_block =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                    dst_edge_block->get_prev_pointer());
            if (!dst_edge_block) {
              break;
            } else {
              dst_entries = dst_edge_block->get_entries();
              auto dst_num_entries = dst_edge_block->get_num_entries();
              dst_entries_cursor =
                  dst_entries - dst_num_entries;  // at the begining
              dst_segment_idx++;
            }
          }
        }
      }
    }

  } else if (fid !=
             graph_store
                 ->get_local_pid()) {  // is outer vertex of this fragment
    auto v_label = parser.GetLabelId(vid);
    uint64_t ov = graph_store->get_lid(v_label, vid);
    if (ov == uint64_t(-1)) {
      return;
    }

    auto v_offset = parser.GetOffset(vid);
    auto max_outer_id_offset =
        (((vertex_t) 1) << parser.GetOffsetWidth()) - (vertex_t) 1;
    seggraph::SegGraph* src_graph = graph_store->get_ov_graph(v_label);
    auto real_lid = parser.GenerateId(0, v_label, max_outer_id_offset - ov);
    graph_store->delete_outer(v_label,
                              real_lid);  // delete vertex from vertex table
    src_graph->add_deleted_outer_num(1);

    // delete ralated edges
    auto src_writer =
        src_graph->create_graph_writer(write_epoch);  // write epoch

    segid_t segid = src_graph->get_vertex_seg_id(ov);
    uint32_t segidx = src_graph->get_vertex_seg_idx(ov);

    VegitoSegmentHeader* segment;

    // process outgoing edges
    for (auto elabel = 0;
         elabel < graph_store->get_schema().edge_relation.size(); elabel++) {
      segment = src_writer.locate_segment(segid, elabel, seggraph::EOUT);

      if (segment == nullptr) {
        continue;
      }

      uintptr_t edge_block_pointer = segment->get_region_ptr(segidx);
      VegitoEdgeBlockHeader* edge_block =
          src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
              edge_block_pointer);
      uintptr_t epoch_table_pointer = segment->get_epoch_table(segidx);
      EpochBlockHeader* epoch_table =
          src_graph->get_block_manager().convert<EpochBlockHeader>(
              epoch_table_pointer);
      if (!edge_block || !epoch_table) {
        continue;
      }
      size_t num_entries = edge_block->get_num_entries();
      VegitoEdgeEntry* entries = edge_block->get_entries();
      VegitoEdgeEntry* entries_cursor = entries - num_entries;
      std::vector<uint64_t> prefix_sum;
      VegitoEdgeBlockHeader* cur_header = edge_block;
      while (cur_header) {
        prefix_sum.push_back(cur_header->get_num_entries());
        cur_header =
            src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                cur_header->get_prev_pointer());
      }

      prefix_sum[0] = 0;
      for (auto i = 0; i < prefix_sum.size() - 1; i++) {
        prefix_sum[i] = 0;
        for (auto j = i + 1; j < prefix_sum.size(); j++) {
          prefix_sum[i] += prefix_sum[j];
        }
      }
      prefix_sum[prefix_sum.size() - 1] = 0;

      int segment_idx = 0;
      std::priority_queue<size_t> delete_offsets;
      std::vector<uint64_t> delete_loc;
      std::vector<seggraph::vertex_t> delete_vertices;
      // find edges
      while (true) {
        while (entries_cursor != entries) {
          seggraph::vertex_t vid = entries_cursor->get_dst();
          auto delete_flag = vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
          if (delete_flag == 1) {
            auto delete_offset_mask =
                (((seggraph::vertex_t) 1)
                 << (sizeof(seggraph::vertex_t) * 8 - 1)) -
                (seggraph::vertex_t) 1;
            auto delete_offset = vid & delete_offset_mask;
            delete_offsets.push(delete_offset);
          } else {
            if (delete_offsets.empty()) {
              delete_vertices.push_back(vid);
              delete_loc.push_back(entries - entries_cursor - 1 +
                                   prefix_sum[segment_idx]);
            } else if (delete_offsets.top() != (entries - entries_cursor - 1 +
                                                prefix_sum[segment_idx])) {
              delete_vertices.push_back(vid);
              delete_loc.push_back(entries - entries_cursor - 1 +
                                   prefix_sum[segment_idx]);
            } else {
              delete_offsets.pop();
            }
          }
          entries_cursor++;
        }
        edge_block =
            src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                edge_block->get_prev_pointer());
        if (!edge_block) {
          break;
        } else {
          entries = edge_block->get_entries();
          auto num_entries = edge_block->get_num_entries();
          entries_cursor = entries - num_entries;  // at the begining
          segment_idx++;
        }
      }

      // delete edges
      uint64_t edge_prop_bytes = graph_store->get_edge_prop_total_bytes(
          elabel + graph_store->get_total_vertex_label_num());
      char* prop_buffer = reinterpret_cast<char*>(malloc(edge_prop_bytes));
      memset(prop_buffer, 0, edge_prop_bytes);
      std::string buf(prop_buffer, edge_prop_bytes);
      std::string_view edge_data(buf);
      free(prop_buffer);
      for (auto idx = 0; idx < delete_loc.size(); idx++) {
        auto dst_offset = parser.GetOffset(delete_vertices[idx]);
        auto dst_label = parser.GetLabelId(delete_vertices[idx]);
        auto mask = ((seggraph::vertex_t) 1)
                    << (sizeof(seggraph::vertex_t) * 8 - 1);
        auto dst_loc = delete_loc[idx] | mask;
        src_writer.put_edge(v_offset, elabel, seggraph::EOUT, dst_loc,
                            edge_data);
        // we does not need process edges between outer vertices
        if (dst_offset < graph_store->get_vtable_max_inner(dst_label)) {
          seggraph::SegGraph* dst_graph =
              graph_store->get_graph<seggraph::SegGraph>(dst_label);
          auto dst_writer = dst_graph->create_graph_writer(write_epoch);
          segid_t dst_segid = dst_graph->get_vertex_seg_id(dst_offset);
          uint32_t dst_segidx = dst_graph->get_vertex_seg_idx(dst_offset);
          VegitoSegmentHeader* dst_segment =
              dst_writer.locate_segment(dst_segid, elabel, seggraph::EIN);
          uintptr_t dst_edge_block_pointer =
              dst_segment->get_region_ptr(dst_segidx);
          VegitoEdgeBlockHeader* dst_edge_block =
              dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                  dst_edge_block_pointer);
          std::vector<uint64_t> dst_prefix_sum;
          VegitoEdgeBlockHeader* dst_cur_header = dst_edge_block;
          while (dst_cur_header) {
            dst_prefix_sum.push_back(dst_cur_header->get_num_entries());
            dst_cur_header =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
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

          size_t dst_num_entries = dst_edge_block->get_num_entries();
          VegitoEdgeEntry* dst_entries = dst_edge_block->get_entries();
          VegitoEdgeEntry* dst_entries_cursor = dst_entries - dst_num_entries;
          bool is_founded = false;

          while (true) {
            while (dst_entries_cursor != dst_entries) {
              seggraph::vertex_t vid = dst_entries_cursor->get_dst();

              auto dst_delete_flag =
                  vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
              if ((max_outer_id_offset - parser.GetOffset(vid)) == ov &&
                  dst_delete_flag != 1) {
                is_founded = true;
                auto del_loc = dst_entries - dst_entries_cursor - 1 +
                               dst_prefix_sum[dst_segment_idx];
                auto mask = ((seggraph::vertex_t) 1)
                            << (sizeof(seggraph::vertex_t) * 8 - 1);
                del_loc = del_loc | mask;
                dst_writer.put_edge(dst_offset, elabel, seggraph::EIN, del_loc,
                                    edge_data);
                break;
              }
              dst_entries_cursor++;
            }
            if (is_founded) {
              break;
            }
            dst_edge_block =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                    dst_edge_block->get_prev_pointer());
            if (!dst_edge_block) {
              break;
            } else {
              dst_entries = dst_edge_block->get_entries();
              auto dst_num_entries = dst_edge_block->get_num_entries();
              dst_entries_cursor =
                  dst_entries - dst_num_entries;  // at the begining
              dst_segment_idx++;
            }
          }
          if (is_founded == false) {
            std::cout << "ERROR edge not founded" << std::endl;
          }
        }
      }
    }

    // process incoming egdes
    for (auto elabel = 0;
         elabel < graph_store->get_schema().edge_relation.size(); elabel++) {
      segment = src_writer.locate_segment(segid, elabel, seggraph::EIN);
      if (segment == nullptr) {
        continue;
      }
      uintptr_t edge_block_pointer = segment->get_region_ptr(segidx);
      VegitoEdgeBlockHeader* edge_block =
          src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
              edge_block_pointer);
      uintptr_t epoch_table_pointer = segment->get_epoch_table(segidx);
      EpochBlockHeader* epoch_table =
          src_graph->get_block_manager().convert<EpochBlockHeader>(
              epoch_table_pointer);
      if (!edge_block || !epoch_table) {
        continue;
      }
      size_t num_entries = edge_block->get_num_entries();
      VegitoEdgeEntry* entries = edge_block->get_entries();
      VegitoEdgeEntry* entries_cursor = entries - num_entries;
      std::vector<uint64_t> prefix_sum;
      VegitoEdgeBlockHeader* cur_header = edge_block;

      while (cur_header) {
        prefix_sum.push_back(cur_header->get_num_entries());
        cur_header =
            src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                cur_header->get_prev_pointer());
      }

      prefix_sum[0] = 0;
      for (auto i = 0; i < prefix_sum.size() - 1; i++) {
        prefix_sum[i] = 0;
        for (auto j = i + 1; j < prefix_sum.size(); j++) {
          prefix_sum[i] += prefix_sum[j];
        }
      }
      prefix_sum[prefix_sum.size() - 1] = 0;

      int segment_idx = 0;
      std::priority_queue<size_t> delete_offsets;
      std::vector<uint64_t> delete_loc;
      std::vector<seggraph::vertex_t> delete_vertices;

      // find edges
      while (true) {
        while (entries_cursor != entries) {
          seggraph::vertex_t vid = entries_cursor->get_dst();
          auto delete_flag = vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
          if (delete_flag == 1) {
            auto delete_offset_mask =
                (((seggraph::vertex_t) 1)
                 << (sizeof(seggraph::vertex_t) * 8 - 1)) -
                (seggraph::vertex_t) 1;
            auto delete_offset = vid & delete_offset_mask;
            delete_offsets.push(delete_offset);
          } else {
            if (delete_offsets.empty()) {
              delete_vertices.push_back(vid);
              delete_loc.push_back(entries - entries_cursor - 1 +
                                   prefix_sum[segment_idx]);
            } else if (delete_offsets.top() != (entries - entries_cursor - 1 +
                                                prefix_sum[segment_idx])) {
              delete_vertices.push_back(vid);
              delete_loc.push_back(entries - entries_cursor - 1 +
                                   prefix_sum[segment_idx]);
            } else {
              delete_offsets.pop();
            }
          }
          entries_cursor++;
        }
        edge_block =
            src_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                edge_block->get_prev_pointer());
        if (!edge_block) {
          break;
        } else {
          entries = edge_block->get_entries();
          auto num_entries = edge_block->get_num_entries();
          entries_cursor = entries - num_entries;  // at the begining
          segment_idx++;
        }
      }

      // delete edges
      uint64_t edge_prop_bytes = graph_store->get_edge_prop_total_bytes(
          elabel + graph_store->get_total_vertex_label_num());
      char* prop_buffer = reinterpret_cast<char*>(malloc(edge_prop_bytes));
      memset(prop_buffer, 0, edge_prop_bytes);
      std::string buf(prop_buffer, edge_prop_bytes);
      std::string_view edge_data(buf);
      free(prop_buffer);
      for (auto idx = 0; idx < delete_loc.size(); idx++) {
        auto dst_offset = parser.GetOffset(delete_vertices[idx]);
        auto dst_label = parser.GetLabelId(delete_vertices[idx]);
        auto mask = ((seggraph::vertex_t) 1)
                    << (sizeof(seggraph::vertex_t) * 8 - 1);
        auto dst_loc = delete_loc[idx] | mask;
        src_writer.put_edge(v_offset, elabel, seggraph::EIN, dst_loc,
                            edge_data);
        assert(dst_offset < graph_store->get_vtable_max_inner(dst_label));

        if (dst_offset < graph_store->get_vtable_max_inner(dst_label)) {
          seggraph::SegGraph* dst_graph =
              graph_store->get_graph<seggraph::SegGraph>(dst_label);
          auto dst_writer = dst_graph->create_graph_writer(write_epoch);
          segid_t dst_segid = dst_graph->get_vertex_seg_id(dst_offset);
          uint32_t dst_segidx = dst_graph->get_vertex_seg_idx(dst_offset);
          VegitoSegmentHeader* dst_segment =
              dst_writer.locate_segment(dst_segid, elabel, seggraph::EOUT);
          assert(dst_segment != nullptr);
          uintptr_t dst_edge_block_pointer =
              dst_segment->get_region_ptr(dst_segidx);
          VegitoEdgeBlockHeader* dst_edge_block =
              dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                  dst_edge_block_pointer);
          std::vector<uint64_t> dst_prefix_sum;
          VegitoEdgeBlockHeader* dst_cur_header = dst_edge_block;
          while (dst_cur_header) {
            dst_prefix_sum.push_back(dst_cur_header->get_num_entries());
            dst_cur_header =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
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

          size_t dst_num_entries = dst_edge_block->get_num_entries();
          VegitoEdgeEntry* dst_entries = dst_edge_block->get_entries();
          VegitoEdgeEntry* dst_entries_cursor = dst_entries - dst_num_entries;
          bool is_founded = false;

          while (true) {
            while (dst_entries_cursor != dst_entries) {
              seggraph::vertex_t vid = dst_entries_cursor->get_dst();
              auto dst_delete_flag =
                  vid >> (sizeof(seggraph::vertex_t) * 8 - 1);
              if ((max_outer_id_offset - parser.GetOffset(vid)) == ov &&
                  dst_delete_flag != 1) {
                is_founded = true;
                auto del_loc = dst_entries - dst_entries_cursor - 1 +
                               dst_prefix_sum[dst_segment_idx];

                auto mask = ((seggraph::vertex_t) 1)
                            << (sizeof(seggraph::vertex_t) * 8 - 1);
                del_loc = del_loc | mask;
                dst_writer.put_edge(dst_offset, elabel, seggraph::EOUT, del_loc,
                                    edge_data);
                break;
              }
              dst_entries_cursor++;
            }
            if (is_founded) {
              break;
            }
            dst_edge_block =
                dst_graph->get_block_manager().convert<VegitoEdgeBlockHeader>(
                    dst_edge_block->get_prev_pointer());
            if (!dst_edge_block) {
              break;
            } else {
              dst_entries = dst_edge_block->get_entries();
              auto dst_num_entries = dst_edge_block->get_num_entries();
              dst_entries_cursor =
                  dst_entries - dst_num_entries;  // at the begining
              dst_segment_idx++;
            }
          }
          if (is_founded == false) {
            LOG(ERROR) << " edge not founded error ######";
          }
        }
      }
    }
  }
}  // NOLINT(readability/fn_size)

}  // namespace graph
}  // namespace gart
