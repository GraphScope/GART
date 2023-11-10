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

#include "graph/ddl.h"
#include "common/util/likely.h"
#include "util/util.h"

namespace gart {
namespace graph {

const int RGMapping::NO_EXIST = -1;
const int RGMapping::INIT_VEC_SZ = 128;

RGMapping::RGMapping(int p_id)
    : p_id_(p_id),
      table2vlabel(INIT_VEC_SZ, NO_EXIST),
      vlabel2table(INIT_VEC_SZ, NO_EXIST),
      key2vids_(INIT_VEC_SZ),
      vid2keys_(INIT_VEC_SZ),
      key2vids_lock_(INIT_VEC_SZ),
      edges_(INIT_VEC_SZ) {
  for (EdgeMeta& meta : edges_) {
    meta.src_vlabel = meta.dst_vlabel = NO_EXIST;
  }
}

void RGMapping::define_vertex(int vertex_label, int table_id) {
  bool expand = util::insert_vec(table2vlabel, table_id, vertex_label,
                                 static_cast<int>(NO_EXIST));
  if (expand) {
    key2vids_.resize(table2vlabel.size());
    vid2keys_.resize(table2vlabel.size());
    key2vids_lock_.resize(table2vlabel.size());
  }
  vertex_label_num_++;
  util::insert_vec(vlabel2table, vertex_label, table_id,
                   static_cast<int>(NO_EXIST));
}

void RGMapping::add_vprop_mapping(int vertex_label, int vprop_id, int col_id) {
  col2vprop_[vertex_label][col_id] = vprop_id;
  vprop2col_[vertex_label][vprop_id] = col_id;
}

int RGMapping::get_vprop2col(int vertex_label, int vprop_id) {
  if (vprop2col_[vertex_label].count(vprop_id)) {
    return vprop2col_[vertex_label][vprop_id];
  } else {
    return NO_EXIST;
  }
}

// one to many
void RGMapping::define_1n_edge(int edge_label, int src_vlabel, int dst_vlabel,
                               int fk_col, bool undirected,
                               size_t edge_prop_size) {
  while (edges_.size() <= edge_label) {
    edges_.emplace_back();
    EdgeMeta& meta = edges_.back();
    meta.src_vlabel = meta.dst_vlabel = NO_EXIST;
  }

  EdgeMeta& meta = edges_[edge_label];
  meta.src_vlabel = src_vlabel;
  meta.dst_vlabel = dst_vlabel;
  meta.src_fk_col = fk_col;
  meta.dst_fk_col = NO_EXIST;  // XXX: only support one direct
  meta.undirected = undirected;
  meta.edge_prop_size = edge_prop_size;

  vlabel2elabel_[{src_vlabel, dst_vlabel}] = edge_label;
}

void RGMapping::define_nn_edge(int edge_label, int src_vlabel, int dst_vlabel,
                               int src_fk_col, int dst_fk_col, bool undirected,
                               size_t edge_prop_size) {
  while (edges_.size() <= edge_label) {
    edges_.emplace_back();
    EdgeMeta& meta = edges_.back();
    meta.src_vlabel = meta.dst_vlabel = NO_EXIST;
  }

  EdgeMeta& meta = edges_[edge_label];
  meta.src_vlabel = src_vlabel;
  meta.dst_vlabel = dst_vlabel;
  meta.src_fk_col = src_fk_col;
  meta.dst_fk_col = dst_fk_col;
  meta.undirected = undirected;
  meta.edge_prop_size = edge_prop_size;

  vlabel2elabel_[{src_vlabel, dst_vlabel}] = edge_label;
}

void RGMapping::add_eprop(int edge_label, int prop_id, int col_id) {}

}  // namespace graph
}  // namespace gart
