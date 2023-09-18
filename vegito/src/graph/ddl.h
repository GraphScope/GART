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

#ifndef VEGITO_SRC_GRAPH_DDL_H_
#define VEGITO_SRC_GRAPH_DDL_H_

#include <cassert>
#include <cstddef>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

#define USE_TBB_MAP 1

#if USE_TBB_MAP
#include "tbb/concurrent_unordered_map.h"
#else
#include "util/util.h"
#endif

namespace gart {
namespace graph {

struct EdgeMeta {
  int src_vlabel;
  int dst_vlabel;
  int src_fk_col;  // col_id of source node keys (e.g., OL_O_ID in ORLI)
  int dst_fk_col;  // only used in many-to-many
  size_t edge_prop_size = 0;
  bool undirected = false;
};

class RGMapping {
 public:
  explicit RGMapping(int p_id);

  /* VERTEX MAPPING */

  // define vertex label (table_id -> vertex_label)
  void define_vertex(int vertex_label, int table_id);

  // define vertex property
  void add_vprop_mapping(int vertex_label, int prop_id, int col_id);
  int get_vprop2col(int vertex_label, int vprop_id);

  /* EDGE MAPPING */

  // one to many
  void define_1n_edge(int edge_label, int src_vlabel, int dst_vlabel,
                      int fk_col, bool undirected = false,
                      size_t edge_prop_size = 0);

  // many to many
  void define_nn_edge(int edge_label, int src_vlabel, int dst_vlabel,
                      int src_fk_col, int dst_fk_col, bool undirected = false,
                      size_t edge_prop_size = 0);

  // define edge property
  void add_eprop(int edge_label, int prop_id, int col_id);

  inline int getPID() const { return p_id_; }

  // get vertex mapping
  inline int get_vlabel(int table_id) const { return table2vlabel[table_id]; }

  inline int get_table(int vlabel) const { return vlabel2table[vlabel]; }

  inline const std::vector<EdgeMeta>& get_edge_metas() const { return edges_; }

  inline int get_elabel_from_vlabel(int src_vlabel, int dst_vlabel) {
    return vlabel2elabel_[{src_vlabel, dst_vlabel}];
  }

  inline const EdgeMeta& get_edge_meta(int elabel) const {
    assert(elabel < edges_.size());
    return edges_[elabel];
  }

  inline const size_t get_edge_label_num() const {
    return vlabel2elabel_.size();
  }

  inline const size_t get_vertex_label_num() const { return vertex_label_num_; }

  // return edge_label
  inline int get_edge_meta(int dst_vlabel, EdgeMeta& meta) const {
    for (int i = 0; i < edges_.size(); ++i) {
      const EdgeMeta& m = edges_[i];
      if (m.dst_vlabel == dst_vlabel) {
        meta = m;
        return i;
      }
    }
    return NO_EXIST;
  }

#if USE_TBB_MAP
  inline void set_key_and_vid(int table_id, uint64_t key, uint64_t vid) {
    assert(table_id < key2vids_.size());
    key2vids_[table_id].insert({key, vid});
    vid2keys_[table_id].insert({vid, key});
  }

  inline uint64_t get_key2vid(int table_id, uint64_t key) const {
    uint64_t ret;
    auto got = key2vids_[table_id].find(key);
    if (got != key2vids_[table_id].end()) {
      ret = got->second;
      return ret;
    } else {
      return UINT64_MAX;
    }
  }

  inline uint64_t get_vid2key(int table_id, uint64_t vid) const {
    uint64_t ret;
    auto got = vid2keys_[table_id].find(vid);
    if (got == vid2keys_[table_id].end())
      ret = 0;
    else
      ret = got->second;
    return ret;
  }
#else
  inline void set_key_and_vid(int table_id, uint64_t key, uint64_t vid) {
    assert(table_id < key2vids_.size());
    util::lock32(&key2vids_lock_[table_id]);
    key2vids_[table_id][key] = vid;
    vid2keys_[table_id][vid] = key;
    util::unlock32(&key2vids_lock_[table_id]);
  }

  inline uint64_t get_key2vid(int table_id, uint64_t key) const {
    uint64_t ret;
    uint32_t* lp = const_cast<uint32_t*>(&key2vids_lock_[table_id]);
    util::lock32(lp);
    std::unordered_map<uint64_t, uint64_t>::const_iterator got =
        key2vids_[table_id].find(key);
    assert(got != key2vids_[table_id].end());
    ret = got->second;
    util::unlock32(lp);
    return ret;
  }

  inline uint64_t get_vid2key(int table_id, uint64_t vid) const {
    uint64_t ret;
    uint32_t* lp = const_cast<uint32_t*>(&key2vids_lock_[table_id]);
    util::lock32(lp);
    std::unordered_map<uint64_t, uint64_t>::const_iterator got =
        vid2keys_[table_id].find(vid);
    if (got == vid2keys_[table_id].end())
      ret = 0;
    else
      ret = got->second;
    util::unlock32(lp);
    return ret;
  }
#endif

 private:
  static const int NO_EXIST = -1;
  static const int INIT_VEC_SZ = 128;

 private:
  const int p_id_;

  std::vector<int> table2vlabel;  // table id -> vertex label
  std::vector<int> vlabel2table;  // vertex label -> table id

  // <vlabel> -> [<vprop id> -> <tp col>]
  std::unordered_map<int, std::unordered_map<int, int>> vprop2col_;
  // <vlabel> -> [<tp col> -> <vprop id>]
  std::unordered_map<int, std::unordered_map<int, int>> col2vprop_;

  std::vector<EdgeMeta> edges_;

  // <src_vlabel, dst_vlabel> -> elabel
  std::map<std::pair<int, int>, int> vlabel2elabel_;

  size_t vertex_label_num_ = 0;

#if USE_TBB_MAP
  using map_t = tbb::concurrent_unordered_map<uint64_t, uint64_t>;
#else
  using map_t = std::unordered_map<uint64_t, uint64_t>;
#endif
  std::vector<uint32_t> key2vids_lock_;  // only used when USE_TBB_MAP is false

  std::vector<map_t> key2vids_;  // table_id -> <key, vid>
  std::vector<map_t> vid2keys_;  // table_id -> <vid, key>
};

}  // namespace graph
}  // namespace gart

#endif  // VEGITO_SRC_GRAPH_DDL_H_
