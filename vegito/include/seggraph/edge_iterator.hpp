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

#pragma once

#include "seggraph/segment_graph.hpp"

namespace seggraph {
class EdgeIteratorBase {
 public:
  virtual bool valid() = 0;
  virtual void next() = 0;
  virtual vertex_t dst_id() = 0;
  virtual bool empty() = 0;
  virtual size_t size() = 0;
  virtual std::string_view edge_data() = 0;
  virtual size_t edge_data_index() { return 0; }

  virtual std::string_view edge_data_from_index(size_t index) {
    return std::string_view();
  }
};

class EpochEdgeIterator : public EdgeIteratorBase {
 public:
  EpochEdgeIterator(VegitoSegmentHeader* _seg_header,
                    VegitoEdgeBlockHeader* _header,
                    EpochBlockHeader* _epoch_header,
                    const BlockManager& _block_manager, size_t _num_entries,
                    size_t _edge_prop_size, timestamp_t _read_epoch_id)
      : seg_header(_seg_header),
        header(_header),
        block_manager(_block_manager),
        num_entries(_num_entries),
        edge_prop_size(_edge_prop_size),
        epoch_header(_epoch_header),
        read_epoch_id(_read_epoch_id) {
    if (header && epoch_header) {
      init(epoch_header);
      // init edge property access
      seg_block_size = seg_header->get_block_size();
      edge_prop_offset = seg_header->get_allocated_edge_num((uintptr_t) header);
    }
  }

  EpochEdgeIterator() = default;

  EpochEdgeIterator(const EpochEdgeIterator& other) = default;

  EpochEdgeIterator& operator=(const EpochEdgeIterator& other) = default;

  EpochEdgeIterator(EpochEdgeIterator&& other) = default;

  EpochEdgeIterator& operator=(EpochEdgeIterator&& other) = default;

  void init(EpochBlockHeader* epoch_header) {
    // search epoch table for the offset
    auto epoch_entries = epoch_header->get_entries();
    auto num_epoches = epoch_header->get_num_entries();
    size_t read_end_offset = -1;
    for (int idx = 0; idx < num_epoches; idx++) {
      auto epoch_cursor = epoch_entries - num_epoches + idx;
      if (read_epoch_id >= epoch_cursor->get_epoch()) {
        // latest epoch
        if (idx == 0) {
          entries = header->get_entries();
          entries_cursor = entries - num_entries;  // at the begining
          return;                                  // nothing to do
        } else {
          auto last_cursor = (epoch_cursor - 1);
          read_end_offset = last_cursor->get_offset();
          break;
        }
      }
    }
    // no edges to read
    if (read_end_offset == -1) {
      header = nullptr;
      entries = nullptr;
      entries_cursor = nullptr;
    } else {
      while (header->get_prev_num_entries() >= read_end_offset) {
        header = block_manager.convert<VegitoEdgeBlockHeader>(
            header->get_prev_pointer());
      }
      auto offset = read_end_offset - header->get_prev_num_entries();
      entries = header->get_entries();
      entries_cursor = entries - offset;
    }
  }

  bool switch_block() {
    // handle empty edge iterator
    if (!header)
      return false;
    header = block_manager.convert<VegitoEdgeBlockHeader>(
        header->get_prev_pointer());
    if (!header)
      return false;
    entries = header->get_entries();
    auto num_entries = header->get_num_entries();

    entries_cursor = entries - num_entries;  // at the begining
    edge_prop_offset = seg_header->get_allocated_edge_num((uintptr_t) header);
    return true;
  }

  bool valid() {
    if (unlikely(entries_cursor == entries))
      return switch_block();
    else
      return true;
  }

  void next() { entries_cursor++; }

  vertex_t dst_id() {
    if (!valid())
      return SegGraph::VERTEX_TOMBSTONE;
    return entries_cursor->get_dst();
  }

  bool empty() { return !header || !epoch_header; }

  // NOTICE: Side effect!!!
  size_t size() {
    // empty iterator
    if (!header || !epoch_header)
      return 0;

    // TODO(ssj): save state
    auto curr_entries_cursor = entries_cursor;
    size_t sz = 0;
    init(epoch_header);
    while (valid()) {
      sz++;
      next();
    }
    // TODO(ssj): restore state
    entries_cursor = curr_entries_cursor;
    return sz;
  }

  std::string_view edge_data() {
    char* data = reinterpret_cast<char*>(
        (uintptr_t) seg_header + seg_block_size -
        (edge_prop_offset + entries - entries_cursor) * edge_prop_size);
    return std::string_view(data, edge_prop_size);
  }

  size_t edge_data_index() override {
    return (edge_prop_offset + entries - entries_cursor);
  }

  std::string_view edge_data_from_index(size_t index) {
    char* data = reinterpret_cast<char*>(
        (uintptr_t) seg_header + seg_block_size - index * edge_prop_size);
    return std::string_view(data, edge_prop_size);
  }

  // TODO(ssj): set corresponding members in EpochEdgeIterator
  // (e.g., entries_cursor, entries) according to `offset`.
  // We do not need to restore state after invoking.
  // offset means the offset-th edge of iter.
  void set_iter_state(int64_t offset) {}

 private:
  VegitoSegmentHeader* seg_header = nullptr;
  VegitoEdgeEntry* entries = nullptr;
  VegitoEdgeEntry* entries_cursor = nullptr;
  VegitoEdgeBlockHeader* header = nullptr;
  EpochBlockHeader* epoch_header = nullptr;

  const BlockManager& block_manager;

  size_t num_entries;

  size_t seg_block_size;
  size_t edge_prop_offset;
  size_t edge_prop_size;
  timestamp_t read_epoch_id;
};
}  // namespace seggraph
