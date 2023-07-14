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

#include "parser.h"

#include <glog/logging.h>
#include <cassert>
#include <fstream>

#include "vineyard/common/util/json.h"
#include "yaml-cpp/yaml.h"

using namespace std;
using json = vineyard::json;

namespace {
template <typename T>
inline void append_str(string& base, const T& append, char delimiter = '|') {
  if (base.empty()) {
    base = to_string(append);
  } else {
    base += delimiter + to_string(append);
  }
}

template <>
inline void append_str<string>(string& base, const string& append,
                               char delimiter) {
  if (base.empty()) {
    base = append;
  } else {
    base += delimiter + append;
  }
}
}  // namespace

namespace converter {

string LogEntry::to_string() const {
  string base;
  base = op_type == OpType::INSERT   ? "add"
         : op_type == OpType::UPDATE ? "update"
         : op_type == OpType::DELETE ? "delete"
                                     : "unknown";
  if (entity_type == EntityType::VERTEX) {
    append_str(base, string("vertex"), '_');
  } else {
    append_str(base, string("edge"), '_');
  }
  append_str(base, epoch);

  if (entity_type == EntityType::VERTEX) {
    append_str(base, vertex.gid);
  } else {
    append_str(base, edge.elabel);
    append_str(base, edge.src_gid);
    append_str(base, edge.dst_gid);
  }

  for (const string& str : properties) {
    append_str(base, str);
  }

  return base;
}

void TxnLogParser::init(const string& rgmapping_file, int subgraph_num) {
  subgraph_num_ = subgraph_num;

  ifstream rg_mapping_file_stream(rgmapping_file);
  if (!rg_mapping_file_stream.is_open()) {
    LOG(ERROR) << "RGMapping file (" << rgmapping_file << ") open failed."
               << "Not exist or permission denied.";
    exit(1);
  }

  YAML::Node rg_mapping;
  try {
    rg_mapping = YAML::LoadFile(rgmapping_file);
  } catch (YAML::ParserException& e) {
    LOG(ERROR) << "RGMapping file (" << rgmapping_file << ") parse failed."
               << "Error message: " << e.what();
    exit(1);
  }
  YAML::Node vdef = rg_mapping["vertexMappings"]["vertex_types"];
  YAML::Node edef = rg_mapping["edgeMappings"]["edge_types"];
  assert(vdef.IsSequence() && edef.IsSequence());
  vlabel_num_ = vdef.size();
  int elabel_num = edef.size();

  std::map<std::string, int> vertex_label2ids;  // vlabel name -> vlabel id

  // parse vertex
  for (int idx = 0; idx < vlabel_num_; ++idx) {
    int id = idx;
    const string& table_name = vdef[idx]["dataSourceName"].as<string>();
    const string& label = vdef[idx]["type_name"].as<string>();
    table2vlabel_.emplace(table_name, id);
    vertex_id_columns_.emplace(table_name,
                               vdef[idx]["idFieldName"].as<string>());
    vertex_label2ids.emplace(label, id);
    YAML::Node properties = vdef[idx]["mappings"];
    vector<string> required_prop_names;
    for (uint64_t prop_id = 0; prop_id < properties.size(); prop_id++) {
      const string& prop_name =
          properties[prop_id]["dataField"]["name"].as<string>();
      required_prop_names.emplace_back(prop_name);
    }
    required_properties_.emplace(table_name, required_prop_names);
  }

  // parse edges
  for (int idx = 0; idx < elabel_num; ++idx) {
    auto table_name = edef[idx]["dataSourceName"].as<string>();
    table2elabel_.emplace(table_name, idx);
    auto src_label = edef[idx]["type_pair"]["source_vertex"].as<string>();
    auto dst_label = edef[idx]["type_pair"]["destination_vertex"].as<string>();
    auto src_col =
        edef[idx]["sourceVertexMappings"][0]["dataField"]["name"].as<string>();
    auto dst_col =
        edef[idx]["destinationVertexMappings"][0]["dataField"]["name"]
            .as<string>();
    edge_label_columns_.emplace(table_name, make_pair(src_col, dst_col));
    auto src_label_id = vertex_label2ids.find(src_label)->second;
    auto dst_label_id = vertex_label2ids.find(dst_label)->second;
    edge_label2src_dst_labels_.push_back(make_pair(src_label_id, dst_label_id));
    YAML::Node properties = edef[idx]["dataFieldMappings"];
    vector<string> required_prop_names;
    for (uint64_t prop_id = 0; prop_id < properties.size(); prop_id++) {
      auto prop_name = properties[prop_id]["dataField"]["name"].as<string>();
      required_prop_names.emplace_back(prop_name);
    }
    required_properties_.emplace(table_name, required_prop_names);
  }

  id_parser_.Init(subgraph_num_, vlabel_num_);
  string_oid2gid_maps_.resize(vlabel_num_);
  int64_oid2gid_maps_.resize(vlabel_num_);
  vertex_nums_.resize(vlabel_num_, 0);
  vertex_nums_per_fragment_.resize(vlabel_num_);
  for (auto idx = 0; idx < vlabel_num_; ++idx) {
    vertex_nums_per_fragment_[idx].resize(subgraph_num_, 0);
  }
}

bool TxnLogParser::parse(LogEntry& out, const string& log_str, int epoch) {
  out.properties.clear();
  out.valid = false;
  out.epoch = epoch;

  // parse JSON
  json log;
  try {
    log = json::parse(log_str);
  } catch (json::exception& e) {
    LOG(ERROR) << "TxnLog parse failed. Error message: " << e.what();
    return false;
  }

// skip unused tables
#ifndef USE_DEBEZIUM
  string table_name = log["table"].get<string>();
#else
  string table_name = log["source"]["table"].get<string>();
#endif
  auto table2vlabel_it = table2vlabel_.find(table_name);
  auto table2elabel_it = table2elabel_.find(table_name);
  if (table2vlabel_it == table2vlabel_.end() &&
      table2elabel_it == table2elabel_.end()) {
    return false;
  }
#ifndef USE_DEBEZIUM
  string type = log["type"].get<string>();
#else
  string type = log["op"].get<string>();
#endif
  if (table2elabel_it != table2elabel_.end()) {
    if (table2vlabel_it != table2vlabel_.end()) {
      LOG(ERROR) << "Table name conflict: " << table_name;
      return false;
    }
    out.entity_type = LogEntry::EntityType::EDGE;
  } else {
    out.entity_type = LogEntry::EntityType::VERTEX;
  }

#ifndef USE_DEBEZIUM
  out.op_type = type == "insert"   ? LogEntry::OpType::INSERT
                : type == "update" ? LogEntry::OpType::UPDATE
                : type == "delete" ? LogEntry::OpType::DELETE
                                   : LogEntry::OpType::UNKNOWN;
#else
  out.op_type = type == "c"   ? LogEntry::OpType::INSERT
                : type == "u" ? LogEntry::OpType::UPDATE
                : type == "d" ? LogEntry::OpType::DELETE
                : type == "r" ? LogEntry::OpType::INSERT
                              : LogEntry::OpType::UNKNOWN;
#endif
  if (out.op_type == LogEntry::OpType::UNKNOWN) {
    LOG(ERROR) << "Unknown operation type: " << type;
    return false;
  }

  if (out.entity_type == LogEntry::EntityType::VERTEX) {
    fill_vertex(out, log);
  } else {
    fill_edge(out, log);
  }

  fill_prop(out, log);

  out.valid = true;
#ifdef USE_DEBEZIUM
  if (type == "r") {
    return false;
  } else {
    return true;
  }
#else
  // TODO(wanglei): currently only debezium support bulkload
  return true;
#endif
}

void TxnLogParser::fill_vertex(LogEntry& out, const json& log) {
#ifndef USE_DEBEZIUM
  const string& table_name(log["table"].get<string>());
  const json& data = log["data"];
#else
  const string& table_name(log["source"]["table"].get<string>());
  json data;
  if (out.op_type == LogEntry::OpType::INSERT) {
    data = log["after"];
  } else if (out.op_type == LogEntry::OpType::DELETE) {
    data = log["before"];
  } else if (out.op_type == LogEntry::OpType::UPDATE) {
    ;
  }
#endif

  const string& vid_col = vertex_id_columns_.find(table_name)->second;
  int vertex_label_id = table2vlabel_.find(table_name)->second;

  int64_t gid;
  if (out.op_type == LogEntry::OpType::INSERT) {
    // partition by round-robin
    int64_t fid = vertex_nums_[vertex_label_id] % subgraph_num_;
    int64_t offset = vertex_nums_per_fragment_[vertex_label_id][fid];
    vertex_nums_[vertex_label_id]++;
    vertex_nums_per_fragment_[vertex_label_id][fid]++;

    gid = id_parser_.GenerateId(fid, vertex_label_id, offset);
    if (data[vid_col].is_number_integer()) {
      int64_oid2gid_maps_[vertex_label_id].emplace(data[vid_col].get<int64_t>(),
                                                   gid);
    } else if (data[vid_col].is_string()) {
      string_oid2gid_maps_[vertex_label_id].emplace(data[vid_col].get<string>(),
                                                    gid);
    }
  } else {
    if (data[vid_col].is_number_integer()) {
      auto it = int64_oid2gid_maps_[vertex_label_id].find(
          data[vid_col].get<int64_t>());
      if (it == int64_oid2gid_maps_[vertex_label_id].end()) {
        LOG(ERROR) << "Vertex id not found: " << data[vid_col].get<int64_t>();
      }
      gid = it->second;
    } else if (data[vid_col].is_string()) {
      auto it = string_oid2gid_maps_[vertex_label_id].find(
          data[vid_col].get<string>());
      if (it == string_oid2gid_maps_[vertex_label_id].end()) {
        LOG(ERROR) << "Vertex id not found: " << data[vid_col].get<string>();
      }
      gid = it->second;
    } else {
      LOG(ERROR) << "Unknown vertex id type: " << data[vid_col].type_name();
      gid = -1;
    }
  }
  out.vertex.gid = gid;
}

void TxnLogParser::fill_edge(LogEntry& out, const json& log) const {
#ifndef USE_DEBEZIUM
  const string& table_name(log["table"].get<string>());
  const json& data = log["data"];
#else
  const string& table_name(log["source"]["table"].get<string>());
  json data;
  if (out.op_type == LogEntry::OpType::INSERT) {
    data = log["after"];
  } else if (out.op_type == LogEntry::OpType::DELETE) {
    data = log["before"];
  } else if (out.op_type == LogEntry::OpType::UPDATE) {
    ;
  }
#endif

  out.edge.elabel = table2elabel_.find(table_name)->second;

  auto edge_col = edge_label_columns_.find(table_name)->second;
  string src_name = edge_col.first;
  string dst_name = edge_col.second;
  auto edge_label_id = table2elabel_.find(table_name)->second;
  int src_label_id = edge_label2src_dst_labels_[edge_label_id].first;
  int dst_label_id = edge_label2src_dst_labels_[edge_label_id].second;

  int64_t src_gid, dst_gid;
  if (data[src_name].is_number_integer()) {
    src_gid = int64_oid2gid_maps_[src_label_id]
                  .find(data[src_name].get<int64_t>())
                  ->second;
  } else if (data[src_name].is_string()) {
    src_gid = string_oid2gid_maps_[src_label_id]
                  .find(data[src_name].get<string>())
                  ->second;
  } else {
    LOG(ERROR) << "Unknown src type: " << data[src_name].type_name();
    src_gid = -1;
  }

  if (data[dst_name].is_number_integer()) {
    dst_gid = int64_oid2gid_maps_[dst_label_id]
                  .find(data[dst_name].get<int64_t>())
                  ->second;
  } else if (data[dst_name].is_string()) {
    dst_gid = string_oid2gid_maps_[dst_label_id]
                  .find(data[dst_name].get<string>())
                  ->second;
  } else {
    LOG(ERROR) << "Unknown dst type: " << data[dst_name].type_name();
    dst_gid = -1;
  }

  out.edge.src_gid = src_gid;
  out.edge.dst_gid = dst_gid;
}

void TxnLogParser::fill_prop(LogEntry& out, const json& log) const {
#ifndef USE_DEBEZIUM
  const string& table_name(log["table"].get<string>());
  const json& data = log["data"];
#else
  const string& table_name(log["source"]["table"].get<string>());
  json data;
  if (out.op_type == LogEntry::OpType::INSERT) {
    data = log["after"];
  } else if (out.op_type == LogEntry::OpType::DELETE) {
    data = log["before"];
  } else if (out.op_type == LogEntry::OpType::UPDATE) {
    ;
  }
#endif
  auto iter = required_properties_.find(table_name);
  const vector<string>& required_prop_names = iter->second;

  for (size_t prop_id = 0; prop_id < required_prop_names.size(); prop_id++) {
    const string& prop_name = required_prop_names[prop_id];
    json prop_value;
    prop_value = data[prop_name];
    string prop_str;
    if (prop_value.is_string()) {
      prop_str = prop_value.get<string>();
    } else if (prop_value.is_number_integer()) {
      prop_str = to_string(prop_value.get<int>());
    } else if (prop_value.is_number_float()) {
      prop_str = to_string(prop_value.get<float>());
    } else {
      LOG(ERROR) << "Unsupported "
                    "property type: "
                 << prop_value.type_name();
      assert(false);
      continue;
    }
    out.properties.push_back(prop_str);
  }
}

}  // namespace converter