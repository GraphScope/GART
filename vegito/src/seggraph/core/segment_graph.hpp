/*
 *
 * The file seggraph/core/segment_graph.hpp is referred and derived from project
 * livegraph,
 *
 *    https://github.com/thu-pacman/LiveGraph
 *
 * which has the following license:
 *
 * Copyright 2020 Guanyu Feng, Tsinghua University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <tbb/concurrent_queue.h>

#include <algorithm>
#include <tuple>
#include <vector>

#include "fragment/shared_storage.h"
#include "graph/ddl.h"

#include "seggraph/core/allocator.hpp"
#include "seggraph/core/block_manager.hpp"
#include "seggraph/core/futex.hpp"

namespace seggraph {
class SegEdgeIterator;
class EpochEdgeIterator;
class SegTransaction;
class EpochGraphWriter;
class EpochGraphReader;

class SegGraph {
 public:
  SegGraph(gart::graph::RGMapping* rg_map,
           size_t _max_block_size = 1 * (1ul << 30),
           vertex_t _max_vertex_id = 1 * (1ul << 20))
      : epoch_id(0),
        transaction_id(0),
        vertex_id(0),
        read_epoch_table(NO_TRANSACTION),
        recycled_vertex_ids(),
        max_vertex_id(_max_vertex_id),

        // segment
        seg_id(0),
        max_seg_id(_max_vertex_id / VERTEX_PER_SEG),

        array_allocator(false),
        block_manager(_max_block_size),

        rg_map(rg_map) {
    array_allocator.set_client(block_manager.get_client());

    auto futex_allocater =
        std::allocator_traits<decltype(array_allocator)>::rebind_alloc<Futex>(
            array_allocator);
    vertex_futexes = futex_allocater.allocate(max_vertex_id);

    auto shared_mutex_allocater =
        std::allocator_traits<decltype(array_allocator)>::rebind_alloc<
            std::shared_timed_mutex*>(array_allocator);
    seg_mutexes = shared_mutex_allocater.allocate(max_seg_id);

    auto pointer_allocater = std::allocator_traits<
        decltype(array_allocator)>::rebind_alloc<uintptr_t>(array_allocator);
    vertex_ptrs = pointer_allocater.allocate(max_vertex_id);

    edge_label_ptrs =
        pointer_allocater.allocate_v6d(max_seg_id, edge_label_ptrs_oid);

    gart::ArrayMeta meta(edge_label_ptrs_oid, max_seg_id);
    blob_schema.set_block_oid(block_manager.get_block_oid());
    blob_schema.set_elabel2segs(meta);

    // tricky method: avoid corner case in segment lock
    seg_mutexes[0] = new std::shared_timed_mutex();
    edge_label_ptrs[0] = block_manager.NULLPOINTER;
    srand(time(0));
  }

  SegGraph(const SegGraph&) = delete;

  SegGraph(SegGraph&&) = delete;

  ~SegGraph() noexcept {
    auto futex_allocater =
        std::allocator_traits<decltype(array_allocator)>::rebind_alloc<Futex>(
            array_allocator);
    futex_allocater.deallocate(vertex_futexes, max_vertex_id);

    auto shared_mutex_allocater =
        std::allocator_traits<decltype(array_allocator)>::rebind_alloc<
            std::shared_timed_mutex*>(array_allocator);
    shared_mutex_allocater.deallocate(seg_mutexes, max_seg_id);

    auto pointer_allocater = std::allocator_traits<
        decltype(array_allocator)>::rebind_alloc<uintptr_t>(array_allocator);

    pointer_allocater.deallocate(vertex_ptrs, max_vertex_id);

    pointer_allocater.deallocate_v6d(edge_label_ptrs_oid);
  }

  vertex_t get_max_vertex_id() const { return vertex_id; }

  vertex_t get_vertex_capacity() const { return max_vertex_id; }

  uint64_t get_deleted_inner_num() const { return deleted_inner; }

  uint64_t get_deleted_outer_num() const { return deleted_outer; }

  void add_deleted_inner_num(uint64_t num) { deleted_inner += num; }

  void add_deleted_outer_num(uint64_t num) { deleted_outer += num; }

  vertex_t get_seg_start_vid(segid_t seg_id) const {
    return seg_id * VERTEX_PER_SEG;
  }

  vertex_t get_seg_end_vid(segid_t seg_id) const {
    return std::min(vertex_id.load(), (seg_id + 1) * VERTEX_PER_SEG);
  }

  segid_t get_max_seg_id() const { return seg_id; }

  segid_t get_vertex_seg_id(vertex_t _vertex_id) const {
    return _vertex_id / VERTEX_PER_SEG;
  }

  uint32_t get_vertex_seg_idx(vertex_t _vertex_id) const {
    return _vertex_id % VERTEX_PER_SEG;
  }

  BlockManager& get_block_manager() { return block_manager; }

  void recycle_segments(timestamp_t epoch_id);

  SegTransaction begin_transaction();
  SegTransaction begin_read_only_transaction();
  SegTransaction begin_batch_loader();

  // new epoch_based interface
  EpochGraphReader create_graph_reader(timestamp_t read_epoch);
  EpochGraphWriter create_graph_writer(timestamp_t write_epoch);

  size_t get_edge_prop_size(label_t label) {
    assert(rg_map);
    return rg_map->get_edge_meta(static_cast<int>(label)).edge_prop_size;
  }

  /**
   * Analytics interface
   */
  template <class T>
  T* alloc_vertex_array(T init, vertex_t sz = -1) {
    if (sz == -1)
      sz = get_max_vertex_id();
    auto vertex_data_allocater =
        std::allocator_traits<decltype(array_allocator)>::rebind_alloc<T>(
            array_allocator);
    T* vertex_data = vertex_data_allocater.allocate(sz);
    assert(vertex_data);
    for (vertex_t v_i = 0; v_i < sz; v_i++) {
      vertex_data[v_i] = init;
    }
    return vertex_data;
  }

  template <typename T>
  T* dealloc_vertex_array(T* vertex_data) {
    auto vertex_data_allocater =
        std::allocator_traits<decltype(array_allocator)>::rebind_alloc<T>(
            array_allocator);
    vertex_data_allocater.deallocate(vertex_data, get_max_vertex_id());
  }

  gart::BlobSchema& get_blob_schema() { return blob_schema; }

 private:
  using cacheline_padding_t = char[64];

  std::atomic<timestamp_t> epoch_id;
  std::atomic<timestamp_t> transaction_id;
  std::atomic<vertex_t> vertex_id;
  std::atomic<segid_t> seg_id;

  tbb::enumerable_thread_specific<timestamp_t> read_epoch_table;
  tbb::enumerable_thread_specific<
      std::vector<std::tuple<uintptr_t, order_t, timestamp_t>>>
      segments_to_recycle;
  tbb::concurrent_queue<vertex_t> recycled_vertex_ids;

  const vertex_t max_vertex_id;
  const segid_t max_seg_id;

  // memory allocator
  SparseArrayAllocator<void> array_allocator;  // meta data
  BlockManager block_manager;                  // topology data

  Futex* vertex_futexes;
  std::shared_timed_mutex** seg_mutexes;
  uintptr_t* vertex_ptrs;
  uintptr_t* edge_label_ptrs;

  vertex_t* vertex_table;
  vertex_t* ovl2g;

  vineyard::ObjectID edge_label_ptrs_oid;
  vineyard::ObjectID ovl2g_oid;
  vineyard::ObjectID ovg2l_map;

  gart::BlobSchema blob_schema;
  uint64_t deleted_inner = 0;
  uint64_t deleted_outer = 0;

  gart::graph::RGMapping* rg_map;

  constexpr static size_t COMPACTION_CYCLE = 1ul << 20;
  constexpr static size_t RECYCLE_FREQ = 1ul << 16;
  constexpr static size_t LAG_EPOCH_NUMBER = 2;

  constexpr static timestamp_t ROLLBACK_TOMBSTONE = INT64_MAX;
  constexpr static timestamp_t NO_TRANSACTION = -1;
  constexpr static timestamp_t RO_TRANSACTION = ROLLBACK_TOMBSTONE - 1;
  constexpr static vertex_t VERTEX_TOMBSTONE = UINT64_MAX;
  constexpr static auto TIMEOUT = std::chrono::milliseconds(1);
  constexpr static size_t COMPACT_EDGE_BLOCK_THRESHOLD =
      5;  // at least compact 20% edges
  constexpr static label_t MAX_LABEL = UINT16_MAX;

  constexpr static size_t COPY_THRESHOLD_ORDER = 3;

  constexpr static order_t INIT_SEGMENT_ORDER = 20;

  friend class SegEdgeIterator;
  friend class EpochEdgeIterator;
  friend class SegTransaction;
  friend class EpochGraphWriter;
  friend class EpochGraphReader;
};
}  // namespace seggraph
