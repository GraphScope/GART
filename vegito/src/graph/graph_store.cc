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

#include "graph/graph_store.h"

namespace gart {
namespace graph {

GraphStore::~GraphStore() {
  for (const auto& schema : blob_schemas_) {
    vineyard::ObjectID vertex_table_oid = schema.second.get_vertex_table_oid(),
                       ovl2g_oid = schema.second.get_ovl2g_oid();
    array_allocator.deallocate_v6d(vertex_table_oid);
    array_allocator.deallocate_v6d(ovl2g_oid);
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
  std::string schema_str = schema.get_json(false, get_local_pid());
  std::string schema_key =
      FLAGS_meta_prefix + "gart_schema_p" + std::to_string(get_local_pid());
  auto response_task = etcd_client_->put(schema_key, schema_str).get();
  assert(response_task.is_ok());
  std::string gie_schema_str = schema.get_json(true, get_local_pid());
  schema_key =
      FLAGS_meta_prefix + "gart_gie_schema_p" + std::to_string(get_local_pid());
  response_task = etcd_client_->put(schema_key, schema_str).get();
  assert(response_task.is_ok());
  // insert the latest epoch as std::numeric_limits<uint64_t>::max() at first
  std::string latest_epoch = FLAGS_meta_prefix + "gart_latest_epoch_p" +
                             std::to_string(get_local_pid());
  response_task =
      etcd_client_
          ->put(latest_epoch,
                std::to_string(std::numeric_limits<uint64_t>::max()))
          .get();
  assert(response_task.is_ok());
}

void GraphStore::add_vgraph(uint64_t vlabel, RGMapping* rg_map) {
  seg_graphs_[vlabel] = new seggraph::SegGraph(rg_map);
  auto& blob_schema = seg_graphs_[vlabel]->get_blob_schema();

  // add outer CSR and its schema
  ov_seg_graphs_[vlabel] = new seggraph::SegGraph(rg_map);
  auto& ov_schema = ov_seg_graphs_[vlabel]->get_blob_schema();
  blob_schema.set_ov_block_oid(ov_schema.get_block_oid());
  blob_schema.set_ov_elabel2segs(ov_schema.get_elabel2segs());

  // add common information
  blob_schema.set_vlabel(vlabel);

  // vertex_table
  {
    auto alloc = std::allocator_traits<decltype(
        array_allocator)>::rebind_alloc<seggraph::vertex_t>(array_allocator);

    vineyard::ObjectID oid;
    uint64_t max_v = seg_graphs_[vlabel]->get_vertex_capacity() +
                     ov_seg_graphs_[vlabel]->get_vertex_capacity();
    auto& vtable = vertex_tables_[vlabel];
    vtable.table = alloc.allocate_v6d(max_v, oid);
    vtable.max_inner = 0;
    vtable.min_outer = max_v;
    vtable.max_inner_location = 0;
    vtable.min_outer_location = max_v;
    vtable.size = max_v;

    gart::VTableMeta meta(oid, max_v);  // TODO(sijie): update args
    blob_schema.set_vtable_meta(meta);
  }

  // ovl2g
  {
    auto alloc = std::allocator_traits<decltype(
        array_allocator)>::rebind_alloc<uint64_t>(array_allocator);

    vineyard::ObjectID oid;
    uint64_t max_v = ov_seg_graphs_[vlabel]->get_vertex_capacity();
    ovl2gs_[vlabel] = alloc.allocate_v6d(max_v, oid);
    gart::ArrayMeta meta(oid, max_v);
    blob_schema.set_ovl2g_meta(meta);
  }

  blob_schemas_[vlabel] = blob_schema;
}

void GraphStore::add_vprop(uint64_t vlabel, Property::Schema schema) {
  assert(seg_graphs_[vlabel]);

  property_schemas_[vlabel] = schema;
  uint64_t v_capacity = seg_graphs_[vlabel]->get_vertex_capacity();

  switch (schema.store_type) {
  case PROP_COLUMN: {
    property_stores_[vlabel] = new PropertyColPaged(schema, v_capacity);
    assert(blob_schemas_.find(vlabel) != blob_schemas_.end());
    auto& blob_schema = blob_schemas_[vlabel];
    auto p = property_stores_[vlabel];
    blob_schema.set_prop_meta(p->get_blob_metas());
    break;
  }
  case PROP_COLUMN2: {
    property_stores_[vlabel] = new PropertyColArray(schema, v_capacity);
    break;
  }
  default:
    printf("Not implement this property store type: %d\n", schema.store_type);
    assert(false);
  }
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
  }
  blob_epoch_ = blob_epoch;
}

namespace {

struct PropDef {
  PropertyStoreDataType dtype;
  int id;
  std::string name;

  vineyard::json json(bool gie = false) const {
    using json = vineyard::json;
    json res;
    std::string type_str[] = {
        "INVALID",     "BOOL",      "CHAR",       "SHORT",       "INT",    // 4
        "LONG",        "FLOAT",     "DOUBLE",     "STRING",      "BYTES",  // 9
        "INT_LIST",    "LONG_LIST", "FLOAT_LIST", "DOUBLE_LIST",           // 13
        "STRING_LIST", "DATE",      "DATETIME",   "LONGSTRING",  "TEXT"    // 18
    };
    if (gie) {
      type_str[DATE] = type_str[STRING];
      type_str[DATETIME] = type_str[STRING];
      type_str[LONGSTRING] = type_str[STRING];
      type_str[TEXT] = type_str[STRING];
    }
    res["data_type"] = type_str[dtype];
    res["id"] = id;
    res["name"] = name;
    return res;
  }
};

const std::string VERTEX = "VERTEX";  // NOLINT(runtime/string)
const std::string EDGE = "EDGE";      // NOLINT(runtime/string)

struct TypeDef {
  int id;
  // vector<int> index; empty
  std::string label;
  std::vector<PropDef> propertyDefList;
  std::string src_vlabel;  // for edge
  std::string dst_vlabel;  // for edge
  std::string type;        // "VERTEX" or "EDGE"

  vineyard::json json(bool gie = false) const {
    using json = vineyard::json;
    json res;
    res["id"] = id;
    json index_array = json::array();
    res["indexes"] = index_array;
    res["label"] = label;

    // propertyDefList
    json props_array = json::array();
    for (const auto& prop : propertyDefList) {
      props_array.push_back(prop.json(gie));
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

 private:
  static std::string vector2str(const std::vector<int> v) {
    std::string mapping_str = "[";
    for (int i = 0; i < v.size(); ++i) {
      char cstr[20];
      sprintf(cstr, "%d", v[i]);  // NOLINT(runtime/printf)
      mapping_str += std::string(cstr);
      if (i != v.size() - 1)
        mapping_str += ", ";
    }
    mapping_str += "]";
    return mapping_str;
  }
};

struct SchemaJson {
  int partitionNum;
  std::vector<TypeDef> types;
};

}  // namespace

void SchemaImpl::fill_json(void* ptr) const {
  SchemaJson& sj = *reinterpret_cast<SchemaJson*>(ptr);

  sj.partitionNum = 0;  // XXX: hard code
  sj.types.resize(label_id_map.size());

  // Prop
  std::vector<PropDef> props(property_id_map.size());
  for (const auto& pair : property_id_map) {
    std::string pname = pair.first.first;
    int pid = pair.second;
    PropDef& prop = props[pid];
    prop.id = pid;
    prop.name = pname;
  }

  // need to sort map
  std::map<int, std::string> sort_id_label_map;
  for (const auto& pair : label_id_map) {
    sort_id_label_map[pair.second] = pair.first;
  }
  assert(sort_id_label_map.size() == label_id_map.size());

  for (const auto& pair : sort_id_label_map) {
    std::string label = pair.second;
    int label_id = pair.first;
    TypeDef& type = sj.types[label_id];

    bool is_v = label_id < elabel_offset;

    type.id = label_id;
    type.label = label;

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
    }
    type.propertyDefList.assign(props.begin() + pid_begin,
                                props.begin() + pid_end);
    if (!is_v) {
      // TODO(sijie): src_vlabel, dst_vlabel
      const auto& pair = edge_relation.at(label_id);
      type.src_vlabel = sj.types.at(pair.first).label;
      type.dst_vlabel = sj.types.at(pair.second).label;
      assert(type.src_vlabel.size() && type.dst_vlabel.size());
    }

    type.type = is_v ? VERTEX : EDGE;
  }
}

std::string SchemaImpl::get_json(bool gie, int pid) {
  using json = vineyard::json;
  SchemaJson sj;
  // fill
  fill_json(&sj);

  json graph_schema;
  graph_schema["partitionNum"] = 0;
  json props_array = json::array();
  for (const auto& type : sj.types) {
    props_array.push_back(type.json(gie));
  }
  graph_schema["types"] = props_array;

  return graph_schema.dump();
}

void GraphStore::get_blob_json(uint64_t write_epoch) const {
  using json = vineyard::json;
  json blob_schema;
  blob_schema["ipc_socket"] = gart::framework::config.getIPCScoket();
  blob_schema["machine id"] = mid_;
  blob_schema["fid"] = local_pid_;
  blob_schema["fnum"] = local_pnum_;
  blob_schema["epoch"] = write_epoch;
  blob_schema["vertex_label_num"] = blob_schemas_.size();
  auto blob_schemas = fetch_blob_schema(write_epoch);
  json blob_array = json::array();
  for (const auto& pair : blob_schemas) {
    uint64_t vlabel = pair.first;
    auto val = pair.second.json();
    blob_array.push_back(val);
  }
  blob_schema["blob"] = blob_array;

  std::string blob_schema_str = blob_schema.dump();
  std::string blob_json_key =
      FLAGS_meta_prefix + "gart_blob_m" + std::to_string(0) + "_p" +
      std::to_string(local_pid_) + "_e" + std::to_string(write_epoch);

  auto response_task = etcd_client_->put(blob_json_key, blob_schema_str).get();
  assert(response_task.is_ok());

  std::string latest_epoch =
      FLAGS_meta_prefix + "gart_latest_epoch_p" + std::to_string(local_pid_);
  response_task =
      etcd_client_->put(latest_epoch, std::to_string(write_epoch)).get();
  assert(response_task.is_ok());
}

}  // namespace graph
}  // namespace gart
