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

#include "converter/parser.h"

#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "etcd/Client.hpp"
#include "glog/logging.h"
#include "vineyard/common/util/json.h"
#include "yaml-cpp/yaml.h"

using std::ifstream;
using std::map;
using std::string;
using std::vector;

using std::make_pair;
using std::to_string;

using vineyard::json;

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

inline string to_lower_case(const string& input) {
  string output = input;
  for (char& c : output) {
    c = std::tolower(static_cast<unsigned char>(c));
  }
  return output;
}

// for MySQL GTID = source_id:transaction_id
inline int extract_tx_id(const std::string& gtid_str) {
  size_t last_colon = gtid_str.find_last_of(':');
  if (last_colon != std::string::npos) {
    try {
      return stoi(gtid_str.substr(last_colon + 1));
    } catch (const std::invalid_argument& e) {
      LOG(ERROR) << "Invalid argument: no conversion could be performed";
    } catch (const std::out_of_range& e) {
      LOG(ERROR) << "Out of range: the converted value would fall out of the "
                    "range of the result type";
    }
  } else {
    LOG(ERROR) << "Error: GTID format invalid. No colon found." << std::endl;
  }

  return -1;
}

}  // namespace

namespace converter {

LogEntry LogEntry::bulk_load_end() {
  LogEntry entry;
  entry.valid_ = true;
  entry.epoch = 1;  // epoch 1 means bulk load end
  entry.op_type = OpType::BULKLOAD_END;
  return entry;
}

string LogEntry::to_string(int64_t binlog_offset) const {
  string base;
  if (op_type == OpType::BULKLOAD_END) {
    base = "bulkload_end";
    append_str(base, epoch);
    append_str(base, binlog_offset);
    return base;
  }

  base = op_type == OpType::INSERT   ? "add"
         : op_type == OpType::UPDATE ? "update"
         : op_type == OpType::DELETE ? "delete"
                                     : "unknown";
  if (base == "unknown") {
    LOG(ERROR) << "Unknown operation type: " << static_cast<int>(op_type);
    assert(false);
  }
  if (entity_type == EntityType::VERTEX) {
    append_str(base, string("vertex"), '_');
  } else {
    append_str(base, string("edge"), '_');
  }

  append_str(base, epoch);

  if (entity_type == EntityType::VERTEX) {
    append_str(base, vertex.gid);
    append_str(base, external_id);
  } else {
    append_str(base, edge.elabel);
    append_str(base, edge.src_gid);
    append_str(base, edge.dst_gid);
    append_str(base, src_external_id);
    append_str(base, dst_external_id);
  }

  for (const string& str : properties) {
    append_str(base, str);
  }

  append_str(base, binlog_offset);
  return base;
}

gart::Status TxnLogParser::init(const string& etcd_endpoint,
                                const string& etcd_prefix, int subgraph_num) {
  subgraph_num_ = subgraph_num;

  std::shared_ptr<etcd::Client> etcd_client =
      std::make_shared<etcd::Client>(etcd_endpoint);

  std::string rg_mapping_key = etcd_prefix + "gart_rg_mapping_yaml";
  etcd::Response response = etcd_client->get(rg_mapping_key).get();
  if (!response.is_ok()) {
    LOG(ERROR) << "RGMapping file get failed.";
    return gart::Status::GraphSchemaConfigError();
  }
  std::string rg_mapping_str = response.value().as_string();

  YAML::Node rg_mapping;
  try {
    rg_mapping = YAML::Load(rg_mapping_str);
  } catch (YAML::ParserException& e) {
    LOG(ERROR) << "RGMapping file parse failed."
               << "Error message: " << e.what();
    return gart::Status::GraphSchemaConfigError();
  }
  YAML::Node vdef = rg_mapping["vertexMappings"]["vertex_types"];
  YAML::Node edef = rg_mapping["edgeMappings"]["edge_types"];
  assert(vdef.IsSequence() && edef.IsSequence());
  vlabel_num_ = vdef.size();
  int elabel_num = edef.size();

  // parse vertex
  for (int idx = 0; idx < vlabel_num_; ++idx) {
    int id = idx;
    const string& table_name =
        to_lower_case(vdef[idx]["dataSourceName"].as<string>());
    const string& label = vdef[idx]["type_name"].as<string>();
    table2label_names_[table_name].push_back(label);
    useful_tables_.insert(table_name);
    vlable_names_.insert(label);
    vertex_id_columns_.emplace(
        label, to_lower_case(vdef[idx]["idFieldName"].as<string>()));
    vertex_label2ids_.emplace(label, id);
    YAML::Node properties = vdef[idx]["mappings"];
    vector<string> required_prop_names;
    for (uint64_t prop_id = 0; prop_id < properties.size(); prop_id++) {
      const string& prop_name =
          to_lower_case(properties[prop_id]["dataField"]["name"].as<string>());
      required_prop_names.emplace_back(prop_name);
    }
    required_properties_.emplace(label, required_prop_names);
  }

  // parse edges
  for (int idx = 0; idx < elabel_num; ++idx) {
    auto table_name = edef[idx]["dataSourceName"].as<string>();
    table_name = to_lower_case(table_name);
    useful_tables_.insert(table_name);
    auto src_label = edef[idx]["type_pair"]["source_vertex"].as<string>();
    auto dst_label = edef[idx]["type_pair"]["destination_vertex"].as<string>();
    auto label = edef[idx]["type_pair"]["edge"].as<string>();
    table2label_names_[table_name].push_back(label);
    auto src_col =
        edef[idx]["sourceVertexMappings"][0]["dataField"]["name"].as<string>();
    auto dst_col =
        edef[idx]["destinationVertexMappings"][0]["dataField"]["name"]
            .as<string>();
    src_col = to_lower_case(src_col);
    dst_col = to_lower_case(dst_col);
    edge_label_columns_.emplace(label, make_pair(src_col, dst_col));
    elabel_names2elabel_.emplace(label, idx);
    auto src_label_id = vertex_label2ids_.find(src_label)->second;
    auto dst_label_id = vertex_label2ids_.find(dst_label)->second;
    edge_label2src_dst_labels_.push_back(make_pair(src_label_id, dst_label_id));
    YAML::Node properties = edef[idx]["dataFieldMappings"];
    vector<string> required_prop_names;
    for (uint64_t prop_id = 0; prop_id < properties.size(); prop_id++) {
      auto prop_name = properties[prop_id]["dataField"]["name"].as<string>();
      required_prop_names.emplace_back(prop_name);
    }
    required_properties_.emplace(label, required_prop_names);
  }

  id_parser_.Init(subgraph_num_, vlabel_num_);
  string_oid2gid_maps_.resize(vlabel_num_);
  int64_oid2gid_maps_.resize(vlabel_num_);
  vertex_nums_.resize(vlabel_num_, 0);
  vertex_nums_per_fragment_.resize(vlabel_num_);
  for (auto idx = 0; idx < vlabel_num_; ++idx) {
    vertex_nums_per_fragment_[idx].resize(subgraph_num_, 0);
  }

  auto response_task =
      etcd_client->put(etcd_prefix + "converter_is_up", "True").get();
  assert(response_task.is_ok());
  return gart::Status::OK();
}

gart::Status TxnLogParser::parse(LogEntry& out, const string& log_str,
                                 int epoch) {
  out.properties.clear();
  out.valid_ = false;
  out.epoch = epoch;
  out.external_id = "";
  out.src_external_id = "";
  out.dst_external_id = "";

  // parse JSON
  json log;
  try {
    log = json::parse(log_str);
  } catch (json::exception& e) {
    LOG(ERROR) << "TxnLog parse failed. Error message: " << e.what();
    return gart::Status::ParseJsonError();
  }

  string table_name, type;
#ifndef USE_DEBEZIUM
  table_name = log["table"].get<string>();
#else
  table_name = log["source"]["table"].get<string>();
#endif
  auto useful_tables_it = useful_tables_.find(table_name);
  if (useful_tables_it == useful_tables_.end()) {
    // skip unused tables
    if (unused_tables_.find(table_name) == unused_tables_.end()) {
      LOG(INFO) << "Skip unused table: " << table_name;
      unused_tables_.insert(table_name);
    }
    return gart::Status::OK();
  }
#ifndef USE_DEBEZIUM
  type = log["type"].get<string>();
#else
  type = log["op"].get<string>();
#endif
  auto label_name = table2label_names_.find(table_name)->second[out.table_idx];
  auto is_vlabel_name_it = vlable_names_.find(label_name);
  if (is_vlabel_name_it != vlable_names_.end()) {
    out.entity_type = LogEntry::EntityType::VERTEX;
  } else {
    out.entity_type = LogEntry::EntityType::EDGE;
  }

#ifndef USE_DEBEZIUM
  out.op_type = type == "insert"   ? LogEntry::OpType::INSERT
                : type == "update" ? LogEntry::OpType::UPDATE
                : type == "delete" ? LogEntry::OpType::DELETE
                                   : LogEntry::OpType::UNKNOWN;
  out.snapshot = LogEntry::Snapshot::FALSE;
#else
  out.op_type = type == "c"   ? LogEntry::OpType::INSERT
                : type == "u" ? LogEntry::OpType::UPDATE
                : type == "d" ? LogEntry::OpType::DELETE
                : type == "r" ? LogEntry::OpType::INSERT
                              : LogEntry::OpType::UNKNOWN;

  // parse transaction id (tx_id)
  // default for PostgreSQL, -1 for MySQL
  // TODO(SSJ): Hardcode for PostgreSQL and MySQL
  int tx_id = log["source"].value("txId", -1);
  if (tx_id == -1) {
    // MySQL
    const json& gtid_json = log["source"]["gtid"];
    const string& gtid_str =
        gtid_json.is_null() ? string() : gtid_json.get<string>();
    if (!gtid_str.empty()) {
      // parse the format: GTID = source_id:transaction_id
      tx_id = extract_tx_id(gtid_str);
    } else if (type == "r") {
      // for snapshot status, GTID is null
      tx_id = 0;
    } else {
      LOG(ERROR) << "Please open GTID for MySQL.";
    }
  }

  out.tx_id = tx_id;

  // Special snapshot status during snapshot:
  // first: firstRecordInTable && firstTable
  // last: lastRecordInTable && lastTable
  // first_in_data_collection: firstRecordInTable
  // last_in_data_collection: lastRecordInTable
  string snapshot = log["source"]["snapshot"].get<string>();
  out.snapshot = snapshot == "last"    ? LogEntry::Snapshot::LAST
                 : snapshot == "true"  ? LogEntry::Snapshot::TRUE
                 : snapshot == "false" ? LogEntry::Snapshot::FALSE
                                       : LogEntry::Snapshot::OTHER;

  if (type == "r" && snapshot == "false") {
    LOG(ERROR) << "Invalid operation type " << type
               << ", when snapshot status is " << snapshot;
    return gart::Status::OperationError();
  }

  if (out.snapshot == LogEntry::Snapshot::OTHER) {
    LOG(INFO) << "Other snapshot status: " << snapshot;
  }
#endif
  if (out.op_type == LogEntry::OpType::UNKNOWN) {
    LOG(ERROR) << "Unknown operation type: " << type;
    return gart::Status::OperationError();
  }

  if (out.op_type == LogEntry::OpType::UPDATE &&
      out.entity_type == LogEntry::EntityType::EDGE) {
    if (out.update_has_finish_delete == false) {
      out.update_has_finish_delete = true;
      out.op_type = LogEntry::OpType::DELETE;
    } else {
      out.update_has_finish_delete = false;
      out.op_type = LogEntry::OpType::INSERT;
    }
  }

  if (out.entity_type == LogEntry::EntityType::VERTEX) {
    fill_vertex(out, log);
  } else {
    fill_edge(out, log);
  }

  if (out.op_type != LogEntry::OpType::DELETE) {
    fill_prop(out, log);
  }

  if (out.update_has_finish_delete != true) {
    out.table_idx++;
  }
  if (out.table_idx < table2label_names_.find(table_name)->second.size()) {
    out.all_labels_have_process = false;
  } else {
    out.all_labels_have_process = true;
    out.table_idx = 0;
  }

  out.valid_ = true;
  return gart::Status::OK();
}

int64_t TxnLogParser::get_gid(const json& oid, int vlabel) const {
  int64_t gid = -1;

  if (oid.is_number_integer()) {
    auto it = int64_oid2gid_maps_[vlabel].find(oid.get<int64_t>());
    if (it == int64_oid2gid_maps_[vlabel].end()) {
      LOG(ERROR) << "Vertex id not found: " << oid.get<int64_t>();
    } else {
      gid = it->second;
    }
  } else if (oid.is_string()) {
    auto it = string_oid2gid_maps_[vlabel].find(oid.get<string>());
    if (it == string_oid2gid_maps_[vlabel].end()) {
      LOG(ERROR) << "Vertex id not found: " << oid.get<string>();
    } else {
      gid = it->second;
    }
  } else {
    LOG(ERROR) << "Unknown vertex id type: " << oid.type_name();
  }

  return gid;
}

void TxnLogParser::set_gid(const vineyard::json& oid, int vlabel, int64_t gid) {
  if (oid.is_number_integer()) {
    int64_oid2gid_maps_[vlabel][oid.get<int64_t>()] = gid;
  } else if (oid.is_string()) {
    string_oid2gid_maps_[vlabel][oid.get<string>()] = gid;
  } else {
    LOG(ERROR) << "Unknown vertex id type: " << oid.type_name();
  }
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
    data = log["after"];
  }
#endif

  string vlabel_name =
      table2label_names_.find(table_name)->second[out.table_idx];
  const string& vid_col = vertex_id_columns_.find(vlabel_name)->second;
  int vertex_label_id = vertex_label2ids_.find(vlabel_name)->second;

  int64_t gid = 0;
  if (out.op_type == LogEntry::OpType::INSERT) {
    // partition by round-robin
    int64_t fid = vertex_nums_[vertex_label_id] % subgraph_num_;
    int64_t offset = vertex_nums_per_fragment_[vertex_label_id][fid];
    vertex_nums_[vertex_label_id]++;
    vertex_nums_per_fragment_[vertex_label_id][fid]++;

    gid = id_parser_.GenerateId(fid, vertex_label_id, offset);
    set_gid(data[vid_col], vertex_label_id, gid);
    string external_id;
    // TODO(wanglei): now only vertex has external id
    if (data[vid_col].is_number_integer()) {
      external_id = std::to_string(data[vid_col].get<int64_t>());
    } else if (data[vid_col].is_string()) {
      external_id = data[vid_col].get<string>();
    } else {
      LOG(ERROR) << "Unknown vertex id type: " << data[vid_col].type_name();
    }
    if (external_id.empty()) {
      LOG(ERROR) << "Empty external id for vertex: " << vlabel_name;
      assert(false);
    }
    out.external_id = external_id;
  } else if (out.op_type == LogEntry::OpType::DELETE ||
             out.op_type == LogEntry::OpType::UPDATE) {
    gid = get_gid(data[vid_col], vertex_label_id);
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
    data = log["after"];
  }
#endif

  string elable_name =
      table2label_names_.find(table_name)->second[out.table_idx];
  out.edge.elabel = elabel_names2elabel_.find(elable_name)->second;

  auto edge_col = edge_label_columns_.find(elable_name)->second;
  string src_name = edge_col.first;
  string dst_name = edge_col.second;
  auto edge_label_id = elabel_names2elabel_.find(elable_name)->second;
  int src_label_id = edge_label2src_dst_labels_[edge_label_id].first;
  int dst_label_id = edge_label2src_dst_labels_[edge_label_id].second;

  if (data[src_name].is_number_integer()) {
    out.src_external_id = std::to_string(data[src_name].get<int64_t>());
  } else if (data[src_name].is_string()) {
    out.src_external_id = data[src_name].get<string>();
  }

  if (data[dst_name].is_number_integer()) {
    out.dst_external_id = std::to_string(data[dst_name].get<int64_t>());
  } else if (data[dst_name].is_string()) {
    out.dst_external_id = data[dst_name].get<string>();
  }

  int64_t src_gid = get_gid(data[src_name], src_label_id);
  int64_t dst_gid = get_gid(data[dst_name], dst_label_id);

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
    data = log["after"];
  }
#endif
  auto label_name = table2label_names_.find(table_name)->second[out.table_idx];
  auto iter = required_properties_.find(label_name);
  const vector<string>& required_prop_names = iter->second;

  for (size_t prop_id = 0; prop_id < required_prop_names.size(); prop_id++) {
    const string& prop_name = required_prop_names[prop_id];
    json prop_value;
    prop_value = data[prop_name];
    string prop_str;
    if (prop_value.is_string()) {
      prop_str = prop_value.get<string>();
    } else if (prop_value.is_number_integer()) {
      prop_str = to_string(prop_value.get<int64_t>());
    } else if (prop_value.is_number_float()) {
      prop_str = to_string(prop_value.get<float>());
    } else if (prop_value.is_null()) {
      prop_str = "";
    } else {
      LOG(ERROR) << "Unsupported property type: " << prop_value.type_name();
      assert(false);
      continue;
    }
    out.properties.push_back(prop_str);
  }
}

}  // namespace converter
