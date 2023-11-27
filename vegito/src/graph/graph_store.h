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

#ifndef VEGITO_SRC_GRAPH_GRAPH_STORE_H_
#define VEGITO_SRC_GRAPH_GRAPH_STORE_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "etcd/Client.hpp"
#include "etcd/Response.hpp"
#include "glog/logging.h"
#include "vineyard/basic/ds/hashmap_mvcc.h"

#include "fragment/id_parser.h"
#include "memory/buffer_manager.h"
#include "property/property_col_array.h"
#include "property/property_col_paged.h"
#include "seggraph/seggraph.hpp"
#include "system_flags.h"  // NOLINT(build/include_subdir)

namespace gart {

namespace framework {
class Runner;
}  // namespace framework

namespace graph {

using hashmap_t = vineyard::HashmapMVCC<int64_t, int64_t>;

struct SchemaImpl {
  // property name, label_id -> property idx
  std::map<std::pair<std::string, int>, int> property_id_map;
  // label -> property offset
  std::map<int, int> label2prop_offset;
  // label name -> label id
  std::unordered_map<std::string, int> label_id_map;
  // <label id, property idx> -> dtype
  std::map<std::pair<int, int>, property::PropertyDataType> dtype_map;
  // <label id, property idx> -> column_family
  std::map<std::pair<int, int>, int> column_family;
  // <label id, property idx> -> column_family_offset
  std::map<std::pair<int, int>, int> column_family_offset;
  // edge label id -> <src_vlabel, dst_vlabel>
  std::unordered_map<int, std::pair<int, int>> edge_relation;
  // the first id of elabel
  int elabel_offset;
  std::string get_json(int pid);

  std::string get_json4gie(int pid);

 private:
  void fill_json(void* ptr) const;
};

class GraphStore {
 public:
  struct VTable {
    seggraph::vertex_t* table;
    uint64_t size;

    uint64_t max_inner;
    uint64_t min_outer;
    uint64_t max_inner_location;
    uint64_t min_outer_location;
  };

  GraphStore(int local_pid, int mid, int total_partitions,
             gart::framework::Runner* runner)
      : local_pid_(local_pid),
        mid_(mid),
        local_pnum_(total_partitions),
        total_partitions_(total_partitions),
        array_allocator_(),
        key_pid_map_(INIT_VEC_SZ),
        key_off_map_(INIT_VEC_SZ),
        pid_off_map_(INIT_VEC_SZ),
        total_vertex_label_num_(0),
        string_buffer_manager_(array_allocator_.get_client()),
        vprop_buffer_manager_(array_allocator_.get_client()),
        etcd_client_(std::make_shared<etcd::Client>(FLAGS_etcd_endpoint)),
        runner_(runner) {}

  ~GraphStore();

  template <class GraphType>
  GraphType* get_graph(uint64_t vlabel);

  inline property::Property* get_property(uint64_t vlabel) {
    return property_stores_[vlabel];
  }

  inline property::Property* get_property_snapshot(uint64_t vlabel,
                                                   uint64_t version) {
    if (property_stores_snapshots_.count({vlabel, version}))
      return property_stores_snapshots_[{vlabel, version}];
    else
      return nullptr;
  }

  inline void set_schema(SchemaImpl schema) { this->schema_ = schema; }

  inline SchemaImpl get_schema() { return this->schema_; }

  inline uint64_t get_local_pid() const { return local_pid_; }
  inline int get_total_partitions() const { return total_partitions_; }
  inline int get_total_vertex_label_num() const {
    return total_vertex_label_num_;
  }

  void set_vertex_label_num(uint64_t vlabel_num) {
    total_vertex_label_num_ = vlabel_num;
  }
  inline uint64_t get_vtable_max_inner(uint64_t vlabel) {
    return vertex_tables_[vlabel].max_inner;
  }

  void update_offset() {
    for (auto [vlabel, property] : property_stores_) {
      if (property)
        property->updateHeader();
    }
  }

  // return true if the vertex is in the local partition, else false
  bool insert_inner_vertex(int epoch, uint64_t gid, std::string external_id,
                           property::StringViewList& vprop);

  bool update_inner_vertex(int epoch, uint64_t gid,
                           property::StringViewList& vprop);

  void construct_eprop(int elabel, const property::StringViewList& eprop,
                       std::string& out);

  void add_string_buffer(size_t size);

  void add_vprop_buffer(size_t size);

  void add_vgraph(uint64_t vlabel, RGMapping* rg_map);

  void add_vprop(uint64_t vlabel, const property::Property::Schema& schema);

  void update_blob(uint64_t blob_epoch);

  void put_blob_json_etcd(uint64_t write_epoch) const;

  void put_schema();

  void put_schema4gie();

  seggraph::SegGraph* get_ov_graph(uint64_t vlabel) {
    return ov_seg_graphs_[vlabel];
  }

  inline void add_inner(uint64_t vlabel, seggraph::vertex_t lid) {
    VTable& vtable = vertex_tables_[vlabel];
    assert(vtable.max_inner_location != vtable.min_outer_location);
    vtable.table[vtable.max_inner_location] = lid;
    ++vtable.max_inner_location;
    ++vtable.max_inner;
  }

  inline void delete_inner(uint64_t vlabel, seggraph::vertex_t offset) {
    VTable& vtable = vertex_tables_[vlabel];
    gart::IdParser<seggraph::vertex_t> parser;
    parser.Init(get_total_partitions(), get_total_vertex_label_num());
    for (auto i = 0; i < vtable.max_inner_location; i++) {
      auto value = vtable.table[i];
      auto delete_flag = value >> (sizeof(seggraph::vertex_t) * 8 - 1);
      if (delete_flag == 1) {
        continue;
      }
      if (parser.GetOffset(value) == offset) {
        uint64_t delete_mask = ((uint64_t) 1) << (sizeof(uint64_t) * 8 - 1);
        vtable.table[vtable.max_inner_location] = (i | delete_mask);
        ++vtable.max_inner_location;
        break;
      }
    }
  }

  inline void add_outer(uint64_t vlabel, seggraph::vertex_t lid) {
    VTable& vtable = vertex_tables_[vlabel];
    if (vtable.max_inner_location == vtable.min_outer_location) {
      LOG(ERROR) << "Not enough space for outer vertex for vlabel " << vlabel;
      assert(false);
    }
    vtable.table[vtable.min_outer_location - 1] = lid;
    --vtable.min_outer;
    --vtable.min_outer_location;
  }

  inline void delete_outer(uint64_t vlabel, seggraph::vertex_t lid) {
    VTable& vtable = vertex_tables_[vlabel];
    gart::IdParser<seggraph::vertex_t> parser;
    parser.Init(get_total_partitions(), get_total_vertex_label_num());
    for (auto i = vtable.size - 1; i >= vtable.min_outer_location; i--) {
      auto value = vtable.table[i];
      auto delete_flag = value >> (sizeof(seggraph::vertex_t) * 8 - 1);
      if (delete_flag == 1) {
        continue;
      }
      if (value == lid) {
        uint64_t delete_mask = ((uint64_t) 1) << (sizeof(uint64_t) * 8 - 1);
        vtable.table[vtable.min_outer_location - 1] = i | delete_mask;
        --vtable.min_outer_location;
        return;
      }
    }
    LOG(ERROR) << "delete outer error ######";
  }

  inline void insert_blob_schema(uint64_t write_epoch) {
    history_blob_schemas_[write_epoch] = blob_schemas_;
  }

  std::map<uint64_t, gart::BlobSchema> fetch_blob_schema(
      uint64_t write_epoch) const {
    auto iter = history_blob_schemas_.find(write_epoch);
    assert(iter != history_blob_schemas_.end());
    return iter->second;
  }

  inline void set_ovl2g(uint64_t vlabel, uint64_t offset,
                        seggraph::vertex_t gid) {
    ovl2gs_[vlabel][offset] = gid;
  }

  inline void init_ovg2ls(uint64_t vlabel_num) {
    auto v6d_client = array_allocator_.get_client();
    history_ovg2ls_.resize(vlabel_num);
    for (auto idx = 0; idx < vlabel_num; idx++) {
      std::shared_ptr<hashmap_t> hmap;
      VINEYARD_CHECK_OK(hashmap_t::Make(*v6d_client, 1, hmap));
      ovg2ls_.emplace_back(hmap);
    }
  }

  void set_ovg2l(std::shared_ptr<hashmap_t>& hmap, uint64_t v_label,
                 int64_t gid, int64_t lid);

  inline void init_vertex_maps(uint64_t vlabel_num) {
    auto v6d_client = array_allocator_.get_client();
    history_vertex_maps_.resize(vlabel_num);
    for (auto idx = 0; idx < vlabel_num; idx++) {
      std::shared_ptr<hashmap_t> hmap;
      VINEYARD_CHECK_OK(hashmap_t::Make(*v6d_client, 1, hmap));
      vertex_maps_.emplace_back(hmap);
    }
  }

  void set_vertex_map(std::shared_ptr<hashmap_t>& hmap, uint64_t v_label,
                      int64_t oid, int64_t gid);

  void add_global_off(uint64_t vlabel, uint64_t key, int pid) {
    if (vlabel >= key_pid_map_.size()) {
      key_pid_map_.resize(vlabel + 1);
      key_off_map_.resize(vlabel + 1);
      pid_off_map_.resize(vlabel + 1);
    }
    key_pid_map_[vlabel][key] = pid;

    if (pid_off_map_[vlabel].find(pid) == pid_off_map_[vlabel].end()) {
      pid_off_map_[vlabel][pid] = 0;
    }
    int off = pid_off_map_[vlabel][pid]++;
    key_off_map_[vlabel][key] = off;
  }

  // global offset
  void get_pid_off(uint64_t vlabel, uint64_t key, int& pid, int& off) const {
    if (vlabel >= key_pid_map_.size() || vlabel >= key_off_map_.size() ||
        key_pid_map_[vlabel].find(key) == key_pid_map_[vlabel].end() ||
        key_off_map_[vlabel].find(key) == key_off_map_[vlabel].end()) {
      pid = -1;
      off = -1;
      return;
    }

    pid = key_pid_map_[vlabel].at(key);
    off = key_off_map_[vlabel].at(key);
  }

  void set_lid(uint64_t vlabel, uint64_t key, uint64_t off) {
    if (vlabel >= key_lid_map_.size()) {
      key_lid_map_.resize(vlabel + 1);
    }
    key_lid_map_[vlabel][key] = off;
  }

  uint64_t get_lid(uint64_t vlabel, uint64_t key) const {
    if (vlabel >= key_lid_map_.size() ||
        key_lid_map_[vlabel].find(key) == key_lid_map_[vlabel].end()) {
      return uint64_t(-1);
    }
    return key_lid_map_[vlabel].at(key);
  }

  void update_property_bytes() {
    for (auto iter = property_schemas_.begin(); iter != property_schemas_.end();
         iter++) {
      auto v_label = iter->first;
      auto prop_schemas = iter->second;
      int prefix_sum = 0;
      for (int idx = 0; idx < prop_schemas.col_families.size(); idx++) {
        auto vlen = prop_schemas.col_families[idx].vlen;
        property_prefix_bytes_.emplace(std::make_pair(v_label, idx),
                                       prefix_sum);
        prefix_sum += vlen;
      }
      property_bytes_.emplace(v_label, prefix_sum);
    }
  }

  uint64_t get_total_property_bytes(uint64_t vlabel) {
    return property_bytes_[vlabel];
  }

  uint64_t get_prefix_property_bytes(uint64_t vlabel, uint64_t idx) {
    return property_prefix_bytes_[std::make_pair(vlabel, idx)];
  }

  void insert_edge_prop_total_bytes(uint64_t elabel, uint64_t bytes) {
    edge_property_bytes_.emplace(elabel, bytes);
  }

  uint64_t get_edge_prop_total_bytes(uint64_t elabel) {
    return edge_property_bytes_[elabel];
  }

  void insert_edge_prop_prefix_bytes(uint64_t elabel, uint64_t idx,
                                     uint64_t bytes) {
    edge_property_prefix_bytes_.emplace(std::make_pair(elabel, idx), bytes);
  }

  uint64_t get_edge_prop_prefix_bytes(uint64_t elabel, uint64_t idx) {
    return edge_property_prefix_bytes_[std::make_pair(elabel, idx)];
  }

  void insert_edge_property_dtypes(uint64_t elabel, uint64_t idx, int dtype) {
    edge_property_dtypes_.emplace(std::make_pair(elabel, idx), dtype);
  }

  int get_edge_property_dtypes(uint64_t elabel, uint64_t idx) {
    return edge_property_dtypes_[std::make_pair(elabel, idx)];
  }

  void insert_vertex_table_maps(std::string table_name, uint64_t id) {
    vertex_table_maps_.emplace(table_name, id);
  }

  uint64_t get_vertex_table_maps(std::string name) {
    if (vertex_table_maps_.find(name) == vertex_table_maps_.end()) {
      return -1;
    }
    return vertex_table_maps_[name];
  }

  void insert_edge_table_maps(std::string table_name, uint64_t id) {
    edge_table_maps_.emplace(table_name, id);
  }

  uint64_t get_edge_table_maps(std::string name) {
    if (edge_table_maps_.find(name) == edge_table_maps_.end()) {
      return -1;
    }
    return edge_table_maps_[name];
  }

  // Return the offset and length of the string in the string buffer
  inline uint64_t put_cstring(const std::string_view& sv) {
    size_t str_len = sv.length();
    size_t str_offset = string_buffer_manager_.put_cstring(sv);
    return (str_offset << 16) | str_len;
  }

  inline uint64_t put_cstring(const std::string& str) {
    return put_cstring(std::string_view(str));
  }

  inline void get_string(uint64_t key, std::string& output) const {
    size_t str_offset = key >> 16;
    size_t str_len = key & 0xffff;
    string_buffer_manager_.get_string(str_offset, str_len, output);
  }

  void init_edge_bitmap_size(uint64_t elabel_num) {
    edge_bitmap_size_.resize(elabel_num);
  }

  size_t get_edge_bitmap_size(uint64_t elabel) const {
    return edge_bitmap_size_[elabel];
  }

  void set_edge_bitmap_size(uint64_t elabel, size_t size) {
    edge_bitmap_size_[elabel] = size;
  }

  void init_vertex_prop_column_family_map(uint64_t vlabel_num) {
    vertex_prop_column_family_map_.resize(vlabel_num);
  }

  void set_vertex_prop_column_family_map(uint64_t vlabel, uint64_t idx,
                                         size_t cf) {
    vertex_prop_column_family_map_[vlabel].push_back(cf);
  }

  size_t get_vertex_prop_column_family_map(uint64_t vlabel, uint64_t idx) {
    return vertex_prop_column_family_map_[vlabel][idx];
  }

  void init_vertex_prop_offset_in_column_family(uint64_t vlabel_num) {
    vertex_prop_offset_in_column_family_.resize(vlabel_num);
  }

  void set_vertex_prop_offset_in_column_family(uint64_t vlabel, uint64_t idx,
                                               size_t offset) {
    vertex_prop_offset_in_column_family_[vlabel].push_back(offset);
  }

  size_t get_vertex_prop_offset_in_column_family(uint64_t vlabel,
                                                 uint64_t idx) {
    return vertex_prop_offset_in_column_family_[vlabel][idx];
  }

  void init_vertex_prop_id_in_column_family(uint64_t vlabel_num) {
    vertex_prop_id_in_column_family_.resize(vlabel_num);
  }

  void set_vertex_prop_id_in_column_family(uint64_t vlabel, uint64_t idx,
                                           size_t id) {
    vertex_prop_id_in_column_family_[vlabel].push_back(id);
  }

  size_t get_vertex_prop_id_in_column_family(uint64_t vlabel, uint64_t idx) {
    return vertex_prop_id_in_column_family_[vlabel][idx];
  }

  void init_external_id_dtype(uint64_t vlabel_num) {
    external_id_dtype_.resize(vlabel_num);
  }

  void init_external_id_store(uint64_t vlabel_num) {
    external_id_stores_.resize(vlabel_num);
    outer_external_id_stores_.resize(vlabel_num);
  }

  void set_external_id_dtype(uint64_t vlabel, int dtype) {
    external_id_dtype_[vlabel] = dtype;
  }

  int get_external_id_dtype(uint64_t vlabel) const {
    return external_id_dtype_[vlabel];
  }

  uint64_t* get_external_id_store(uint64_t vlabel) {
    return external_id_stores_[vlabel];
  }

  uint64_t* get_outer_external_id_store(uint64_t vlabel) {
    return outer_external_id_stores_[vlabel];
  }

  void init_external_id_storage(uint64_t vlabel);

 private:
  static const int INIT_VEC_SZ = 128;

  const int local_pid_;         // from 0 in each machine
  const int mid_;               // machine id
  const int local_pnum_;        // number of partitions in the machine
  const int total_partitions_;  // total number of partitions
  int total_vertex_label_num_;

  gart::framework::Runner* runner_;

  SparseArrayAllocator array_allocator_;

  // graph store schema
  SchemaImpl schema_;

  // vlabel -> graph storage
  std::unordered_map<uint64_t, seggraph::SegGraph*> seg_graphs_;
  std::unordered_map<uint64_t, seggraph::SegGraph*> ov_seg_graphs_;  // outer v

  // vlabel -> vertex property storage
  std::unordered_map<uint64_t, property::Property*> property_stores_;
  std::unordered_map<uint64_t, property::Property::Schema> property_schemas_;

  // vlabel -> vertex table
  std::unordered_map<uint64_t, VTable> vertex_tables_;

  // outer v: vlabel, offset -> gid
  std::unordered_map<uint64_t, uint64_t*> ovl2gs_;

  // outer v: vlabel -> pointer of lid hashmap
  std::vector<std::shared_ptr<hashmap_t>> ovg2ls_;
  // label ->version -> pointer of lid hashmap
  std::vector<std::vector<std::shared_ptr<hashmap_t>>> history_ovg2ls_;

  // meta data for property fields
  std::map<uint64_t, uint64_t> property_bytes_;
  std::map<std::pair<uint64_t, uint64_t>, uint64_t> property_prefix_bytes_;

  std::map<uint64_t, uint64_t> edge_property_bytes_;
  std::map<std::pair<uint64_t, uint64_t>, uint64_t> edge_property_prefix_bytes_;
  std::map<std::pair<uint64_t, uint64_t>, uint64_t> edge_property_dtypes_;

  std::map<std::string, uint64_t> vertex_table_maps_;
  std::map<std::string, uint64_t> edge_table_maps_;

  // key is vid in SegCSR
  // global id (gid)
  std::vector<std::unordered_map<uint64_t, int>>
      key_pid_map_;  // vlabel -> <key, partition>

  std::vector<std::unordered_map<uint64_t, int>>
      key_off_map_;  // vlabel -> <key, offset>

  std::vector<std::unordered_map<int, int>>
      pid_off_map_;  // vlabel -> <fid, global offset header>

  // local id (lid)
  std::vector<std::unordered_map<uint64_t, uint64_t>>
      key_lid_map_;  // vlabel -> <key, local id>

  // for string buffer
  memory::BufferManager string_buffer_manager_;

  // for vertex property page buffer
  memory::BufferManager vprop_buffer_manager_;

  // for bitmap
  std::vector<size_t> edge_bitmap_size_;

  // for vertex flexible property
  std::vector<std::vector<size_t>> vertex_prop_column_family_map_;
  std::vector<std::vector<size_t>> vertex_prop_offset_in_column_family_;
  std::vector<std::vector<size_t>> vertex_prop_id_in_column_family_;

  // for external id
  std::vector<int> external_id_dtype_;
  std::vector<uint64_t*> external_id_stores_;
  std::vector<uint64_t*> outer_external_id_stores_;
  std::vector<std::shared_ptr<hashmap_t>> vertex_maps_;
  // label ->version -> pointer of vertex hashmap
  std::vector<std::vector<std::shared_ptr<hashmap_t>>> history_vertex_maps_;

  // vlabel -> vertex blob schemas
  std::map<uint64_t, gart::BlobSchema> blob_schemas_;
  std::map<uint64_t, std::map<uint64_t, gart::BlobSchema>>
      history_blob_schemas_;  // version --> map<vlabel, schema>

  uint64_t blob_epoch_;

  std::shared_ptr<etcd::Client> etcd_client_;

  // (vlabel, version) -> vertex property storage snapshot
  std::map<std::pair<uint64_t, uint64_t>, property::Property*>
      property_stores_snapshots_;
};

}  // namespace graph
}  // namespace gart

#endif  // VEGITO_SRC_GRAPH_GRAPH_STORE_H_
