/*
 *
 * The file seggraph/src/segment_graph.cpp is referred and derived from project
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

#include "seggraph/segment_graph.hpp"
#include "seggraph/epoch_graph_reader.hpp"
#include "seggraph/epoch_graph_writer.hpp"

using EpochGraphReader = seggraph::EpochGraphReader;
using EpochGraphWriter = seggraph::EpochGraphWriter;
using SegGraph = seggraph::SegGraph;
using timestamp_t = seggraph::timestamp_t;

EpochGraphReader SegGraph::create_graph_reader(timestamp_t read_epoch) {
  return EpochGraphReader(*this, read_epoch);
}

EpochGraphWriter SegGraph::create_graph_writer(timestamp_t write_epoch) {
  if (rand() % RECYCLE_FREQ == 0) {  // NOLINT(runtime/threadsafe_fn)
    recycle_segments(write_epoch);
  }
  return EpochGraphWriter(*this, write_epoch);
}

void SegGraph::recycle_segments(timestamp_t epoch_id) {
  std::vector<std::tuple<uintptr_t, order_t, timestamp_t>>
      new_segments_to_recycle;
  for (std::tuple<uintptr_t, order_t, timestamp_t> segment :
       segments_to_recycle.local()) {
    timestamp_t epoch = std::get<2>(segment);
    if (epoch + LAG_EPOCH_NUMBER < epoch_id) {
      block_manager.free(std::get<0>(segment), std::get<1>(segment));
    } else {
      new_segments_to_recycle.push_back(segment);
    }
  }
  segments_to_recycle.local().swap(new_segments_to_recycle);
}
