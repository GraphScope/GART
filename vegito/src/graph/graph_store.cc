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
#include <cstdint>
#include <fstream>

#include "graph/graph_store.h"
#include "property/property.h"
#include "util/bitset.h"

using std::map;
using std::numeric_limits;
using std::string;
using std::vector;

using std::to_string;

using namespace gart::property;  // NOLINT(build/namespaces)

namespace {
struct PropDef {
  PropertyDataType dtype;
  int id;
  string name;
  int column_family_id;
  int column_family_offset;

  vineyard::json json() const {
    using json = vineyard::json;
    json res;
    string type_str[] = {
        "INVALID",     "BOOL",      "CHAR",       "SHORT",
        "INT",  // 4
        "LONG",        "FLOAT",     "DOUBLE",     "STRING",
        "BYTES",                                                  // 9
        "INT_LIST",    "LONG_LIST", "FLOAT_LIST", "DOUBLE_LIST",  // 13
        "STRING_LIST", "DATE",      "DATETIME",   "TIME",         // 17
        "TIMESTAMP"                                               // 18
    };
    res["data_type"] = type_str[dtype];
    res["id"] = id;
    res["name"] = name;
    res["column_family_id"] = column_family_id;
    res["column_family_offset"] = column_family_offset;
    return res;
  }

  vineyard::json json4gie() const {
    using json = vineyard::json;
    json res;
    res["key"]["id"] = id;
    res["key"]["name"] = name;
    int dtype_index = 0;
    if (dtype == PropertyDataType::BOOL) {
      dtype_index = 0;
    } else if (dtype == PropertyDataType::INT) {
      dtype_index = 1;
    } else if (dtype == PropertyDataType::LONG) {
      dtype_index = 2;
    } else if (dtype == PropertyDataType::DOUBLE) {
      dtype_index = 3;
    } else if (dtype == PropertyDataType::STRING) {
      dtype_index = 4;
    } else {
      assert(false);
    }
    res["data_type"] = dtype_index;
    res["is_primary_key"] = false;
    return res;
  }
};

const string VERTEX = "VERTEX";  // NOLINT(runtime/string)
const string EDGE = "EDGE";      // NOLINT(runtime/string)

struct TypeDef {
  int id;
  string label;
  vector<PropDef> propertyDefList;
  string src_vlabel;  // for edge
  string dst_vlabel;  // for edge
  string type;        // "VERTEX" or "EDGE"

  int vertex_label_num;  // TODO(wanglei): for gie, remove later
  int src_vlabel_id;
  int dst_vlabel_id;

  vineyard::json json() const {
    using json = vineyard::json;
    json res;
    res["id"] = id;
    json index_array = json::array();
    res["indexes"] = index_array;
    res["label"] = label;

    // propertyDefList
    json props_array = json::array();
    for (const auto& prop : propertyDefList) {
      props_array.push_back(prop.json());
    }
    res["propertyDefList"] = props_array;

    // rawRelationShips
    json relation = json::object();
    if (type == EDGE) {
      relation["dstVertexLabel"] = dst_vlabel;
      relation["srcVertexLabel"] = src_vlabel;
    }
    json rel_array = json::array();
    rel_array.push_back(relation);
    res["rawRelationShips"] = rel_array;

    res["type"] = type;

    return res;
  }

  vineyard::json json4gie() const {
    using json = vineyard::json;
    json res;
    if (type == VERTEX) {
      res["label"]["id"] = id;
      res["label"]["name"] = label;
      json props_array = json::array();
      for (const auto& prop : propertyDefList) {
        props_array.push_back(prop.json4gie());
      }
      res["columns"] = props_array;
    } else {
      res["label"]["id"] = id - vertex_label_num;
      res["label"]["name"] = label;
      json relation_array = json::array();
      json relation_content;
      relation_content["src"]["id"] = src_vlabel_id;
      relation_content["src"]["name"] = src_vlabel;
      relation_content["dst"]["id"] = dst_vlabel_id;
      relation_content["dst"]["name"] = dst_vlabel;
      relation_array.push_back(relation_content);
      res["entity_pairs"] = relation_array;
      json props_array = json::array();
      for (const auto& prop : propertyDefList) {
        props_array.push_back(prop.json4gie());
      }
      res["columns"] = props_array;
    }
    return res;
  }

 private:
  static string vector2str(const vector<int> v) {
    string mapping_str = "[";
    for (int i = 0; i < v.size(); ++i) {
      char cstr[20];
      sprintf(cstr, "%d", v[i]);  // NOLINT(runtime/printf)
      mapping_str += string(cstr);
      if (i != v.size() - 1)
        mapping_str += ", ";
    }
    mapping_str += "]";
    return mapping_str;
  }
};

struct SchemaJson {
  int partitionNum;
  vector<TypeDef> types;
};

}  // namespace

namespace gart {
namespace graph {

GraphStore::~GraphStore() {
  for (const auto& schema : blob_schemas_) {
    vineyard::ObjectID vertex_table_oid = schema.second.get_vertex_table_oid(),
                       ovl2g_oid = schema.second.get_ovl2g_oid();
    array_allocator_.deallocate_v6d(vertex_table_oid);
    array_allocator_.deallocate_v6d(ovl2g_oid);
  }
}

template <>
seggraph::SegGraph* GraphStore::get_graph<seggraph::SegGraph>(uint64_t vlabel) {
  return seg_graphs_[vlabel];
}

template <class GraphType>
GraphType* GraphStore::get_graph(uint64_t vlabel) {
  return nullptr;
}

void GraphStore::put_schema() {
  auto schema = get_schema();
  string schema_str = schema.get_json(get_local_pid());
  string schema_key =
      FLAGS_meta_prefix + "gart_schema_p" + to_string(get_local_pid());
  auto response_task = etcd_client_->put(schema_key, schema_str).get();
  assert(response_task.is_ok());
  // insert the latest epoch as numeric_limits<uint64_t>::max() at first
  string latest_epoch =
      FLAGS_meta_prefix + "gart_latest_epoch_p" + to_string(get_local_pid());
  response_task =
      etcd_client_
          ->put(latest_epoch, to_string(numeric_limits<uint64_t>::max()))
          .get();
  assert(response_task.is_ok());
  schema_key =
      FLAGS_meta_prefix + "writer_" + to_string(get_local_pid()) + "_is_up";
  response_task = etcd_client_->put(schema_key, "True").get();
  assert(response_task.is_ok());
}

void GraphStore::put_schema4gie() {
  auto schema = get_schema();
  string schema_str = schema.get_json4gie(get_local_pid());
  string schema_key =
      FLAGS_meta_prefix + "gart_gie_schema_p" + to_string(get_local_pid());
  auto response_task = etcd_client_->put(schema_key, schema_str).get();
  assert(response_task.is_ok());
}

void GraphStore::add_string_buffer(size_t size) {
  string_buffer_manager_.init_capacity(size);
}

void GraphStore::add_vprop_buffer(size_t size) {
  vprop_buffer_manager_.init_capacity(size);
}

int block_size_num_v_power[][2] = {{24, 13}, {30, 12}, {31, 14}, {23, 12},
                                   {33, 18}, {36, 26}, {34, 25}, {32, 21}};

void GraphStore::add_vgraph(uint64_t vlabel, RGMapping* rg_map) {
#if 0  // size specified for each vlabel in LDBC
  uint64_t block_size = 1ul << block_size_num_v_power[vlabel][0];
  uint64_t num_vertex = 1ul << block_size_num_v_power[vlabel][1];
  seg_graphs_[vlabel] =
      new seggraph::SegGraph(rg_map, vlabel, block_size, num_vertex);

  // add outer CSR and its schema
  ov_seg_graphs_[vlabel] =
      new seggraph::SegGraph(rg_map, vlabel, block_size, num_vertex);
#else
  uint64_t block_size = get_max_memory_usage(vlabel);
  uint64_t num_vertex = get_max_vertex_num(vlabel);
  seg_graphs_[vlabel] =
      new seggraph::SegGraph(rg_map, vlabel, block_size, num_vertex);

  // add outer CSR and its schema
  ov_seg_graphs_[vlabel] = new seggraph::SegGraph(rg_map, vlabel, block_size,
                                                  num_vertex);
#endif

  auto& blob_schema = seg_graphs_[vlabel]->get_blob_schema();
  auto& ov_schema = ov_seg_graphs_[vlabel]->get_blob_schema();
  blob_schema.set_ov_block_oid(ov_schema.get_block_oid());
  blob_schema.set_ov_elabel2segs(ov_schema.get_elabel2segs());

  // add common information
  blob_schema.set_vlabel(vlabel);

  // vertex_table
  {
    vineyard::ObjectID oid;
    uint64_t max_v = seg_graphs_[vlabel]->get_vertex_capacity() +
                     ov_seg_graphs_[vlabel]->get_vertex_capacity();
    auto& vtable = vertex_tables_[vlabel];
    vtable.table =
        array_allocator_.allocate_v6d<seggraph::vertex_t>(max_v, oid);
    vtable.max_inner = 0;
    vtable.min_outer = max_v;
    vtable.max_inner_location = 0;
    vtable.min_outer_location = max_v;
    vtable.size = max_v;

    gart::VTableMeta meta(oid, max_v);
    blob_schema.set_vtable_meta(meta);
  }

  // ovl2g
  {
    vineyard::ObjectID oid;
    uint64_t max_v = ov_seg_graphs_[vlabel]->get_vertex_capacity();
    ovl2gs_[vlabel] = array_allocator_.allocate_v6d<uint64_t>(max_v, oid);
    gart::ArrayMeta meta(oid, max_v);
    blob_schema.set_ovl2g_meta(meta);
  }

  blob_schemas_[vlabel] = blob_schema;
}

void GraphStore::add_vprop(uint64_t vlabel, const Property::Schema& schema) {
  assert(seg_graphs_[vlabel]);

  property_schemas_[vlabel] = schema;
  uint64_t v_capacity = seg_graphs_[vlabel]->get_vertex_capacity();

  switch (schema.store_type) {
  case PROP_COLUMN: {
    property_stores_[vlabel] =
        new PropertyColPaged(schema, v_capacity, vprop_buffer_manager_);
    assert(blob_schemas_.find(vlabel) != blob_schemas_.end());
    auto& blob_schema = blob_schemas_[vlabel];
    auto p = property_stores_[vlabel];
    blob_schema.set_prop_meta(p->get_blob_metas());
    break;
  }
  case PROP_COLUMN2: {
    property_stores_[vlabel] =
        new PropertyColArray(schema, v_capacity, vprop_buffer_manager_);
    break;
  }
  default:
    printf("Not implement this property store type: %d\n", schema.store_type);
    assert(false);
  }
}

void GraphStore::init_external_id_storage(uint64_t vlabel) {
  vineyard::ObjectID object_id = 0;
  uint64_t v_capacity = seg_graphs_[vlabel]->get_vertex_capacity();
  external_id_stores_[vlabel] =
      array_allocator_.allocate_v6d<uint64_t>(v_capacity, object_id);
  blob_schemas_[vlabel].set_external_id_oid(object_id);
  blob_schemas_[vlabel].set_external_id_dtype(external_id_dtype_[vlabel]);

  uint64_t outer_v_capacity = ov_seg_graphs_[vlabel]->get_vertex_capacity();
  outer_external_id_stores_[vlabel] =
      array_allocator_.allocate_v6d<uint64_t>(outer_v_capacity, object_id);
  blob_schemas_[vlabel].set_outer_external_id_oid(object_id);
}

void GraphStore::update_blob(uint64_t blob_epoch) {
  for (auto& pair : blob_schemas_) {
    uint64_t vlabel = pair.first;
    gart::BlobSchema& schema = pair.second;
    seggraph::SegGraph* graph = seg_graphs_[vlabel];
    seggraph::SegGraph* ov_graph = ov_seg_graphs_[vlabel];
    schema.set_vtable_bound(graph->get_max_vertex_id(),
                            ov_graph->get_max_vertex_id());
    schema.set_vtable_location(
        graph->get_max_vertex_id() + graph->get_deleted_inner_num(),
        ov_graph->get_max_vertex_id() + ov_graph->get_deleted_outer_num());
    schema.set_ovg2l_oid(ovg2ls_[vlabel]->id());
    schema.set_vertex_map_oid(vertex_maps_[vlabel]->id());
    history_vertex_maps_[vlabel] = vertex_maps_[vlabel];
    history_ovg2ls_[vlabel] = ovg2ls_[vlabel];
  }
  blob_epoch_ = blob_epoch;
}

void SchemaImpl::fill_json(void* ptr) const {
  SchemaJson& sj = *reinterpret_cast<SchemaJson*>(ptr);

  sj.partitionNum = 0;  // XXX: hard code
  sj.types.resize(label_id_map.size());

  // Prop
  vector<PropDef> props(property_id_map.size());
  for (const auto& pair : property_id_map) {
    string pname = pair.first.first;
    int pid = pair.second;
    PropDef& prop = props[pid];
    prop.id = pid;
    prop.name = pname;
  }

  // need to sort map
  map<int, string> sort_id_label_map;
  for (const auto& pair : label_id_map) {
    sort_id_label_map[pair.second] = pair.first;
  }
  assert(sort_id_label_map.size() == label_id_map.size());

  for (const auto& pair : sort_id_label_map) {
    string label = pair.second;
    int label_id = pair.first;
    TypeDef& type = sj.types[label_id];

    bool is_v = label_id < elabel_offset;

    type.id = label_id;
    type.label = label;

    // TODO(wanglei): for gie, remove later
    type.vertex_label_num = label_id_map.size() - edge_relation.size();
    if (!is_v) {
      type.src_vlabel_id = edge_relation.at(label_id).first;
      type.dst_vlabel_id = edge_relation.at(label_id).second;
    }

    int pid_begin = -1, pid_end = -1;
    if (label2prop_offset.find(label_id) != label2prop_offset.end()) {
      pid_begin = label2prop_offset.at(label_id);
      auto next = label2prop_offset.lower_bound(label_id + 1);
      if (next != label2prop_offset.end()) {
        pid_end = next->second;
      } else {
        pid_end = props.size();
      }
    }
    for (int pid = pid_begin; pid < pid_end; ++pid) {
      int local_pid = pid - pid_begin;
      props[pid].dtype = dtype_map.at({label_id, local_pid});
      if (label_id < elabel_offset) {
        props[pid].column_family_id = column_family.at({label_id, local_pid});
        props[pid].column_family_offset =
            column_family_offset.at({label_id, local_pid});
      } else {
        props[pid].column_family_id = -1;
        props[pid].column_family_offset = -1;
      }
    }
    type.propertyDefList.assign(props.begin() + pid_begin,
                                props.begin() + pid_end);
    if (!is_v) {
      const auto& pair = edge_relation.at(label_id);
      type.src_vlabel = sj.types.at(pair.first).label;
      type.dst_vlabel = sj.types.at(pair.second).label;
      assert(type.src_vlabel.size() && type.dst_vlabel.size());
    }

    type.type = is_v ? VERTEX : EDGE;
  }
}

string SchemaImpl::get_json(int pid) {
  using json = vineyard::json;
  SchemaJson sj;
  // fill
  fill_json(&sj);

  json graph_schema;
  graph_schema["partitionNum"] = 0;
  json props_array = json::array();
  for (const auto& type : sj.types) {
    props_array.push_back(type.json());
  }
  graph_schema["types"] = props_array;

  return graph_schema.dump();
}

string SchemaImpl::get_json4gie(int pid) {
  using json = vineyard::json;
  SchemaJson sj;
  // fill
  fill_json(&sj);

  json graph_schema;

  json vertex_array = json::array();
  json edge_array = json::array();
  for (const auto& type : sj.types) {
    if (type.type == VERTEX) {
      vertex_array.push_back(type.json4gie());
    } else {
      edge_array.push_back(type.json4gie());
    }
  }
  graph_schema["entities"] = vertex_array;
  graph_schema["relations"] = edge_array;
  graph_schema["is_table_id"] = true;
  graph_schema["is_column_id"] = false;

  return graph_schema.dump();
}

void GraphStore::put_blob_json_etcd(uint64_t write_epoch) {
  using json = vineyard::json;
  json blob_schema;
  blob_schema["ipc_socket"] = gart::framework::config.getIPCScoket();
  blob_schema["machine id"] = mid_;
  blob_schema["fid"] = local_pid_;
  blob_schema["fnum"] = local_pnum_;
  blob_schema["epoch"] = write_epoch;
  blob_schema["instance_id"] = array_allocator_.get_client()->instance_id();
  blob_schema["vertex_label_num"] = blob_schemas_.size();
  std::time_t currentTime = std::time(nullptr);
  // put the currentTime as a int64_t into blob schema
  blob_schema["timestamp"] = currentTime;
  blob_schema["total_vertex_num"] =
      total_vertex_num_.load(std::memory_order_relaxed);
  blob_schema["total_edge_num"] =
      total_edge_num_.load(std::memory_order_relaxed);
  blob_schema["string_buffer_object_id"] =
      string_buffer_manager_.get_buffer_oid();
  auto blob_schemas = fetch_blob_schema(write_epoch);
  json blob_array = json::array();
  for (const auto& pair : blob_schemas) {
    uint64_t vlabel = pair.first;
    auto val = pair.second.json();
    blob_array.push_back(val);
  }
  blob_schema["blob"] = blob_array;

  string blob_schema_str = blob_schema.dump();
  string blob_json_key = FLAGS_meta_prefix + "gart_blob_m" + to_string(0) +
                         "_p" + to_string(local_pid_) + "_e" +
                         to_string(write_epoch);

  auto response_task = etcd_client_->put(blob_json_key, blob_schema_str).get();
  assert(response_task.is_ok());

  string latest_epoch =
      FLAGS_meta_prefix + "gart_latest_epoch_p" + to_string(local_pid_);
  response_task = etcd_client_->put(latest_epoch, to_string(write_epoch)).get();
  assert(response_task.is_ok());
}

bool GraphStore::insert_inner_vertex(int epoch, uint64_t gid,
                                     std::string external_id,
                                     StringViewList& vprop) {
  auto vlabel = id_parser.GetLabelId(gid);
#ifdef USE_GLOBAL_VERTEX_MAP
  // global vertex map
  if (external_id_dtype_[vlabel] == PropertyDataType::LONG) {
    std::shared_ptr<hashmap_t> hmap;
    set_vertex_map(hmap, vlabel, std::stoll(external_id), (int64_t) gid);
  }
#endif

  auto fid = id_parser.GetFid(gid);
  if (fid != local_pid_) {
    return false;  // not in this partition
  }

  add_total_vertex_num_by_one();

#ifndef USE_GLOBAL_VERTEX_MAP
  // local vertex map
  if (external_id_dtype_[vlabel] == PropertyDataType::LONG) {
    std::shared_ptr<hashmap_t> hmap;
    set_vertex_map(hmap, vlabel, std::stoll(external_id), (int64_t) gid);
  }
#endif

  uint64_t voffset = id_parser.GetOffset(gid);
  auto lid = id_parser.GenerateId(0, vlabel, voffset);

  // get handle of graph and property
  seggraph::SegGraph* graph = get_graph<seggraph::SegGraph>(vlabel);

#ifdef USE_MULTI_THREADS
  inner_vertex_label_mutexes_[vlabel]->lock();
#endif

  auto writer = graph->create_graph_writer(epoch);  // write epoch
  Property* property = get_property(vlabel);

// insert the topology
#ifdef USE_MULTI_THREADS
  seggraph::vertex_t v = writer.new_vertex(voffset);
#else
  seggraph::vertex_t v = writer.new_vertex();
  auto off = property->getNewOffset();
  assert(v == off);
#endif

  add_inner(vlabel, lid);

  if (external_id_dtype_[vlabel] == PropertyDataType::STRING) {
    uint64_t value = put_cstring(external_id);
    external_id_stores_[vlabel][v] = value;
  } else {
    external_id_stores_[vlabel][v] = std::stoll(external_id);
  }

#ifdef USE_MULTI_THREADS
  inner_vertex_label_mutexes_[vlabel]->unlock();
#endif

  // deal with string
  // allocate space for string_view
  vector<string> tmp_str(vprop.size());
  for (size_t idx = 0; idx < vprop.size(); ++idx) {
    auto dtype = schema_.dtype_map.at({vlabel, idx});
    if (dtype == STRING) {
      uint64_t value = put_cstring(vprop[idx]);
      tmp_str[idx] = to_string(value);
      vprop[idx] = tmp_str[idx];
    }
  }

  // insert properties
  property->insert(v, gid, vprop, epoch, this, vlabel);

  return true;
}

bool GraphStore::update_inner_vertex(int epoch, uint64_t gid,
                                     StringViewList& vprop) {
  auto fid = id_parser.GetFid(gid);
  if (fid != local_pid_) {
    return false;  // not in this partition
  }

  auto vlabel = id_parser.GetLabelId(gid);
  auto voffset = id_parser.GetOffset(gid);
  Property* property = get_property(vlabel);

  property->update(voffset, gid, vprop, epoch, this);
  return true;
}

void GraphStore::set_vertex_map(std::shared_ptr<hashmap_t>& hmap,
                                uint64_t v_label, int64_t oid, int64_t gid) {
#ifdef USE_MULTI_THREADS
  vertex_label_mutexes_[v_label]->lock();
#endif
  VINEYARD_CHECK_OK(vertex_maps_[v_label]->emplace(hmap, oid, gid));
  if (hmap != nullptr) {
    auto old_hmap = vertex_maps_[v_label];
    auto hmap_in_previous_epoch = history_vertex_maps_[v_label];
    if (old_hmap != hmap_in_previous_epoch) {
      auto v6d_client = array_allocator_.get_client();
      VINEYARD_DISCARD(old_hmap->blob_writer()->Abort(*v6d_client));
    }
    vertex_maps_[v_label] = hmap;
  }
#ifdef USE_MULTI_THREADS
  vertex_label_mutexes_[v_label]->unlock();
#endif
}

void GraphStore::set_ovg2l(std::shared_ptr<hashmap_t>& hmap, uint64_t v_label,
                           int64_t gid, int64_t lid) {
  VINEYARD_CHECK_OK(ovg2ls_[v_label]->emplace(hmap, gid, lid));
  if (hmap != nullptr) {
    auto old_hmap = ovg2ls_[v_label];
    auto hmap_in_previous_epoch = history_ovg2ls_[v_label];
    if (old_hmap != hmap_in_previous_epoch) {
      auto v6d_client = array_allocator_.get_client();
      VINEYARD_DISCARD(old_hmap->blob_writer()->Abort(*v6d_client));
    }
    ovg2ls_[v_label] = hmap;
  }
}

void GraphStore::construct_eprop(int elabel, const StringViewList& eprop,
                                 std::string& out) {
  out.clear();
  out.resize(get_edge_prop_total_bytes(elabel + total_vertex_label_num_) +
             edge_bitmap_size_[elabel]);
  char* prop_buffer = out.data();

  // null bitmap
  uint8_t* bitmap = reinterpret_cast<uint8_t*>(prop_buffer);
  memset(bitmap, 0, edge_bitmap_size_[elabel]);

  string tmp_str;  // for store string_view
  for (size_t idx = 0; idx < eprop.size(); idx++) {
    if (eprop[idx].size() == 0) {
      set_bit(bitmap, idx);
    }

    auto dtype =
        get_edge_property_dtypes(elabel + total_vertex_label_num_, idx);
    uint64_t property_offset =
        get_edge_prop_prefix_bytes(elabel + total_vertex_label_num_, idx);

    // bitmap allocated before properties
    void* prop_ptr = prop_buffer + edge_bitmap_size_[elabel] + property_offset;
    std::string_view sv = eprop[idx];
    if (dtype == STRING) {
      auto str_len = eprop[idx].length();
      if (str_len == 0) {
        set_bit(bitmap, idx);
      }
      uint64_t value = put_cstring(eprop[idx]);
      tmp_str = to_string(value);
      sv = tmp_str;
    }

    if (eprop[idx].size() != 0) {
      Property::assign_prop(dtype, prop_ptr, sv);
    }
  }
}

}  // namespace graph
}  // namespace gart
