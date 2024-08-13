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
#include <stdio.h>
#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "librdkafka/rdkafkacpp.h"
#include "yaml-cpp/yaml.h"

#include "framework/bench_runner.h"
#include "graph/graph_ops.h"
#include "system_flags.h"
#include "util/bitset.h"

using std::ifstream;
using std::map;
using std::string;
using std::string_view;
using std::vector;

using std::cout;
using std::endl;

using namespace gart::property;  // NOLINT(build/namespaces)

namespace {
inline vector<string_view> splitString(const string_view& str, char delimiter) {
  vector<std::string_view> result;
  size_t startPos = 0;
  size_t endPos = str.find(delimiter);

  while (endPos != string::npos) {
    result.push_back(string_view(str.data() + startPos, endPos - startPos));
    startPos = endPos + 1;
    endPos = str.find(delimiter, startPos);
  }

  if (startPos < str.length()) {
    result.push_back(
        string_view(str.data() + startPos, str.length() - startPos));
  } else if (startPos == str.length()) {
    result.push_back(string_view(""));
  }
  return result;
}

inline string toLowerCase(const string& input) {
  string output = input;
  for (char& c : output) {
    c = std::tolower(static_cast<unsigned char>(c));
  }
  return output;
}
}  // anonymous namespace

namespace gart {
namespace framework {

Status init_graph_schema(string etcd_endpoint, string etcd_prefix,
                         graph::GraphStore* graph_store,
                         graph::RGMapping* rg_map) {
  using json = vineyard::json;

  std::shared_ptr<etcd::Client> etcd_client =
      std::make_shared<etcd::Client>(etcd_endpoint);

  std::string rg_mapping_key = etcd_prefix + "gart_rg_mapping_yaml";
  std::string rg_mapping_str;
  etcd::Response response;
  while (true) {
    try {
      response = etcd_client->get(rg_mapping_key).get();
      if (!response.is_ok()) {
        std::cout << "Key '" << rg_mapping_key << "' not found. Retrying..."
                  << std::endl;
      } else {
        rg_mapping_str = response.value().as_string();
        break;
      }
    } catch (std::exception& e) {
      std::cout << "Error accessing etcd: " << e.what() << ". Retrying..."
                << std::endl;
    }
    sleep(5);
  }

  std::string table_schema_key = etcd_prefix + "gart_table_schema";
  std::string table_schema_str;
  while (true) {
    try {
      response = etcd_client->get(table_schema_key).get();
      if (!response.is_ok()) {
        std::cout << "Key '" << table_schema_key << "' not found. Retrying..."
                  << std::endl;
      } else {
        table_schema_str = response.value().as_string();
        break;
      }
    } catch (std::exception& e) {
      std::cout << "Error accessing etcd: " << e.what() << ". Retrying..."
                << std::endl;
    }
    sleep(5);
  }

  YAML::Node config;
  try {
    config = YAML::Load(rg_mapping_str);
  } catch (YAML::ParserException& e) {
    LOG(ERROR) << "Parse graph schema file failed: " << e.what();
    return Status::GraphSchemaConfigError();
  }

  json table_schema;
  try {
    table_schema = json::parse(table_schema_str);
  } catch (json::parse_error& e) {
    LOG(ERROR) << "Parse table schema failed: " << e.what();
    return Status::TableConfigError();
  }

  graph::SchemaImpl graph_schema;
  map<string, int> vertex_name_id_map;

  YAML::Node vdef = config["vertexMappings"]["vertex_types"];
  YAML::Node edef = config["edgeMappings"]["edge_types"];
  assert(vdef.IsSequence() && edef.IsSequence());
  int vlabel_num = vdef.size(), elabel_num = edef.size();

  graph_store->set_vertex_label_num(vlabel_num);
  graph_store->init_id_parser(vlabel_num);
#ifdef USE_MULTI_THREADS
  graph_store->init_mutexes(vlabel_num);
#endif
  graph_schema.elabel_offset = vlabel_num;
  int prop_offset = 0;

  Property::Schema prop_schema;          // for vertex properties
  prop_schema.store_type = PROP_COLUMN;  // use column store

  // alloc string buffer
  // TODO(wanglei): hard code
  graph_store->add_string_buffer((1ul << 30) * 30);  // 30GB
  graph_store->add_vprop_buffer((1ul << 30) * 30);   // 30GB

  // Parse vertex
  for (int idx = 0; idx < vlabel_num; ++idx) {
    int id = idx;
    string name = vdef[idx]["type_name"].as<string>();
    string table_name = vdef[idx]["dataSourceName"].as<string>();
    table_name = toLowerCase(table_name);
    vertex_name_id_map.emplace(name, id);
    graph_schema.label_id_map[name] = id;
    rg_map->define_vertex(id, id);  // (vlabel, table_id)
  }

  graph_store->init_external_id_dtype(vlabel_num);
  graph_store->init_external_id_store(vlabel_num);
  graph_store->init_vertex_prop_column_family_map(vlabel_num);
  graph_store->init_vertex_prop_offset_in_column_family(vlabel_num);
  graph_store->init_vertex_prop_id_in_column_family(vlabel_num);

  // Parse edge
  for (int idx = 0; idx < elabel_num; ++idx) {
    int id = idx + vlabel_num;
    string name = edef[idx]["type_pair"]["edge"].as<string>();
    string src_name = edef[idx]["type_pair"]["source_vertex"].as<string>();
    string dst_name = edef[idx]["type_pair"]["destination_vertex"].as<string>();
    auto src_col =
        edef[idx]["sourceVertexMappings"][0]["dataField"]["name"].as<string>();
    auto dst_col =
        edef[idx]["destinationVertexMappings"][0]["dataField"]["name"]
            .as<string>();

    bool edge_is_undirected =
        edef[idx]["type_pair"]["undirected"].as<bool>(false);
    graph_schema.label_id_map[name] = id;
    int src_label_id = vertex_name_id_map.find(src_name)->second;
    int dst_label_id = vertex_name_id_map.find(dst_name)->second;
    graph_schema.edge_relation[id] = {src_label_id, dst_label_id};
    // TODO(wanglei): fk and is_directed are hard code
    rg_map->define_nn_edge(id, src_label_id, dst_label_id, 0, 0,
                           edge_is_undirected,
                           edef[idx]["dataFieldMappings"].size());
  }

  graph_store->init_edge_bitmap_size(elabel_num);

  for (auto idx = 0; idx < vlabel_num; ++idx) {
    graph_store->set_max_vertex_num(idx, FLAGS_default_max_vertex_number);
    graph_store->set_max_memory_usage(
        idx, FLAGS_default_max_memory_usage_for_each_type_vertex);
  }

  std::cout << "Customized vertex number memory usage config: "
            << FLAGS_customized_vertex_number_memory_usage_config << std::endl;

  if (!FLAGS_customized_vertex_number_memory_usage_config.empty()) {
    string customized_config =
        FLAGS_customized_vertex_number_memory_usage_config;
    vector<string_view> type_configs = splitString(customized_config, ',');
    for (auto& type_config : type_configs) {
      vector<string_view> type_config_parts = splitString(type_config, ':');
      if (type_config_parts.size() != 3) {
        LOG(ERROR) << "Invalid customized vertex number memory usage config: "
                   << type_config;
        return Status::Invalid();
      }
      string type_name(type_config_parts[0]);
      uint64_t vertex_num = std::stoull(string(type_config_parts[1]));
      uint64_t memory_usage = std::stoull(string(type_config_parts[2]));
      int type_id = vertex_name_id_map.find(type_name)->second;
      graph_store->set_max_vertex_num(type_id, vertex_num);
      graph_store->set_max_memory_usage(type_id, memory_usage);
    }
  }

  for (auto idx = 0; idx < vlabel_num; ++idx) {
    graph_store->add_vgraph(idx, rg_map);
  }

  for (int idx = 0; idx < vlabel_num + elabel_num; ++idx) {
    int id = idx;
    string table_name;
    string external_id_col_name;
    YAML::Node prop_info;
    bool is_vertex = (idx < vlabel_num);
    if (is_vertex) {
      prop_info = vdef[idx]["mappings"];
      table_name = vdef[idx]["dataSourceName"].as<string>();
      external_id_col_name = vdef[idx]["idFieldName"].as<string>();
    } else {
      prop_info = edef[idx - vlabel_num]["dataFieldMappings"];
      table_name = edef[idx - vlabel_num]["dataSourceName"].as<string>();
    }
    table_name = toLowerCase(table_name);

    if (prop_info.size() != 0) {
      graph_schema.label2prop_offset[id] = prop_offset;
    }

    if (!is_vertex) {
      auto bitmap_size = BYTE_SIZE(prop_info.size());
      graph_store->set_edge_bitmap_size(id - vlabel_num, bitmap_size);
    }

    uint64_t edge_prop_prefix_bytes = 0;
    auto required_table_schema = table_schema[table_name];

    std::vector<Property::ColumnFamily> column_family_info(prop_info.size());
    std::vector<bool> column_family_info_is_valid(prop_info.size(), false);
    for (auto col_family_idx = 0; col_family_idx < prop_info.size();
         col_family_idx++) {
      column_family_info[col_family_idx].page_size = 0;
      column_family_info[col_family_idx].updatable = true;
      column_family_info[col_family_idx].vlen = 0;
      column_family_info[col_family_idx].column_num = 0;
    }

    for (int prop_idx = 0; prop_idx < prop_info.size(); prop_idx++) {
      int prop_id = prop_idx;
      string prop_name = prop_info[prop_idx]["property"].as<string>();
      string prop_dtype = "";
      string prop_table_col_name =
          prop_info[prop_idx]["dataField"]["name"].as<string>();
      prop_table_col_name = toLowerCase(prop_table_col_name);

      size_t column_family_id = 0;
      if (is_vertex) {
        column_family_id =
            prop_info[prop_idx]["dataField"]["columnFamily"].as<size_t>(
                prop_idx);
        column_family_info_is_valid[column_family_id] = true;
        graph_store->set_vertex_prop_id_in_column_family(
            id, prop_id, column_family_info[column_family_id].column_num);
        column_family_info[column_family_id].column_num++;
        graph_store->set_vertex_prop_column_family_map(id, prop_id,
                                                       column_family_id);
        graph_schema.column_family[{id, prop_id}] = column_family_id;
        graph_schema.column_family_offset[{id, prop_id}] =
            column_family_info[column_family_id].vlen;
        graph_store->set_vertex_prop_offset_in_column_family(
            id, prop_id, column_family_info[column_family_id].vlen);
      }

      for (int col_idx = 0; col_idx < required_table_schema.size(); ++col_idx) {
        if (required_table_schema[col_idx].size() != 2) {
          LOG(ERROR) << "Table schema format error. "
                     << "Table name" << table_name << "Column index " << col_idx
                     << " has " << required_table_schema[col_idx].size()
                     << " columns.";
          assert(false);
        }
        if (required_table_schema[col_idx][0].get<string>() ==
            prop_table_col_name) {
          string prop_dtype_str =
              required_table_schema[col_idx][1].get<string>();
          // TODO(wanglei): support more data types in MySQL and PostgreSQL
          if (prop_dtype_str == "int" || prop_dtype_str == "integer") {
            prop_dtype = "INT";
          } else if (prop_dtype_str == "bigint") {
            prop_dtype = "LONG";
          } else if (prop_dtype_str == "float") {
            prop_dtype = "FLOAT";
          } else if (prop_dtype_str == "double" ||
                     prop_dtype_str == "double precision") {
            prop_dtype = "DOUBLE";
          } else if (prop_dtype_str.rfind("varchar", 0) == 0 ||
                     prop_dtype_str == "character varying" ||
                     prop_dtype_str == "text") {
            prop_dtype = "STRING";
          } else if (prop_dtype_str == "timestamp" ||
                     prop_dtype_str.rfind("timestamp(", 0) == 0 ||
                     prop_dtype_str == "timestamp with time zone") {
            prop_dtype = "TIMESTAMP";
          } else if (prop_dtype_str == "timestamp without time zone") {
            prop_dtype = "TIMESTAMP";
          } else if (prop_dtype_str == "date") {
            prop_dtype = "DATE";
          } else if (prop_dtype_str == "time" ||
                     prop_dtype_str.rfind("time(", 0) == 0 ||
                     prop_dtype_str == "time without time zone") {
            prop_dtype = "TIME";
          } else if (prop_dtype_str == "time with time zone") {
            prop_dtype = "TIME";
          } else if (prop_dtype_str == "datetime" ||
                     prop_dtype_str.rfind("datetime(", 0) == 0) {
            prop_dtype = "DATETIME";
          } else {
            return Status::TypeError();
          }
          break;
        }
      }

      if (prop_dtype == "") {
        LOG(ERROR) << "Table schema format error. "
                   << "Table name " << table_name << " Column name "
                   << prop_table_col_name << " not found.";
        assert(false);
      }

      if (prop_dtype == "INT") {
        graph_schema.dtype_map[{id, prop_id}] = INT;
        if (is_vertex) {
          column_family_info[column_family_id].vlen += sizeof(int);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, INT);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(int);
        }
      } else if (prop_dtype == "FLOAT") {
        graph_schema.dtype_map[{id, prop_id}] = FLOAT;
        if (is_vertex) {
          column_family_info[column_family_id].vlen += sizeof(float);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, FLOAT);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(float);
        }
      } else if (prop_dtype == "DOUBLE") {
        graph_schema.dtype_map[{id, prop_id}] = DOUBLE;
        if (is_vertex) {
          column_family_info[column_family_id].vlen += sizeof(double);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, DOUBLE);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(double);
        }
      } else if (prop_dtype == "LONG") {
        graph_schema.dtype_map[{id, prop_id}] = LONG;
        if (is_vertex) {
          column_family_info[column_family_id].vlen += sizeof(uint64_t);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, LONG);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(uint64_t);
        }
      } else if (prop_dtype == "CHAR") {
        graph_schema.dtype_map[{id, prop_id}] = CHAR;
        if (is_vertex) {
          column_family_info[column_family_id].vlen += sizeof(char);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, CHAR);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(char);
        }
      } else if (prop_dtype == "STRING") {
        graph_schema.dtype_map[{id, prop_id}] = STRING;
        if (is_vertex) {
          column_family_info[column_family_id].vlen += sizeof(uint64_t);
          // use string id (str_offset << 16 | str_len) instead of itself
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, STRING);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(int64_t);
        }
      } else if (prop_dtype == "DATE") {
        graph_schema.dtype_map[{id, prop_id}] = DATE;
        if (is_vertex) {
          column_family_info[column_family_id].vlen +=
              sizeof(gart::graph::Date);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, DATE);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(gart::graph::Date);
        }
      } else if (prop_dtype == "DATETIME") {
        graph_schema.dtype_map[{id, prop_id}] = DATETIME;
        if (is_vertex) {
          column_family_info[column_family_id].vlen +=
              sizeof(gart::graph::DateTime);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, DATETIME);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(gart::graph::DateTime);
        }
      } else if (prop_dtype == "TIME") {
        graph_schema.dtype_map[{id, prop_id}] = TIME;
        if (is_vertex) {
          column_family_info[column_family_id].vlen +=
              sizeof(gart::graph::Time);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, TIME);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(gart::graph::Time);
        }
      } else if (prop_dtype == "TIMESTAMP") {
        graph_schema.dtype_map[{id, prop_id}] = TIMESTAMP;
        if (is_vertex) {
          column_family_info[column_family_id].vlen +=
              sizeof(gart::graph::TimeStamp);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, TIMESTAMP);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(gart::graph::TimeStamp);
        }
      } else {
        return Status::TypeError();
      }

      graph_schema.property_id_map[std::make_pair(prop_name, idx)] =
          prop_offset;
      prop_offset++;
    }

    if (is_vertex) {
      // process external_id
      for (int col_idx = 0; col_idx < required_table_schema.size(); ++col_idx) {
        if (required_table_schema[col_idx][0].get<string>() ==
            external_id_col_name) {
          string prop_dtype_str =
              required_table_schema[col_idx][1].get<string>();
          if (prop_dtype_str.rfind("varchar", 0) == 0 ||
              prop_dtype_str == "character varying" ||
              prop_dtype_str == "text") {
            graph_store->set_external_id_dtype(id, PropertyDataType::STRING);
          } else {
            graph_store->set_external_id_dtype(id, PropertyDataType::LONG);
          }
          break;
        }
      }
    }

    if (is_vertex) {
      for (auto col_family_idx = 0; col_family_idx < column_family_info.size();
           col_family_idx++) {
        if (column_family_info_is_valid[col_family_idx]) {
          prop_schema.col_families.push_back(
              column_family_info[col_family_idx]);
        }
      }
      prop_schema.table_id = id;
      graph_store->add_vprop(id, prop_schema);
      prop_schema.col_families.clear();
    } else {
      graph_store->insert_edge_prop_total_bytes(id, edge_prop_prefix_bytes);
      edge_prop_prefix_bytes = 0;
    }
  }

  for (auto idx = 0; idx < vlabel_num; ++idx) {
    graph_store->init_external_id_storage(idx);
  }

  graph_store->set_schema(graph_schema);
  graph_store->update_property_bytes();
  return Status::OK();
}

#ifdef USE_MULTI_THREADS
void Runner::process_log_thread(int pid, int thread_id) {
  std::string log_str;
  while (true) {
    while (!logs_.try_pop(log_str)) {
      working_state_mutex_[thread_id]->lock_shared();
      bool is_working = is_working_[thread_id];
      working_state_mutex_[thread_id]->unlock_shared();
      if (is_working) {
        working_state_mutex_[thread_id]->lock();
        is_working_[thread_id] = false;
        working_state_mutex_[thread_id]->unlock();
      }
      std::this_thread::yield();
    }
    working_state_mutex_[thread_id]->lock_shared();
    bool is_working = is_working_[thread_id];
    working_state_mutex_[thread_id]->unlock_shared();
    if (!is_working) {
      working_state_mutex_[thread_id]->lock();
      is_working_[thread_id] = true;
      working_state_mutex_[thread_id]->unlock();
    }
    std::string_view log(log_str);
    auto sv_vec = splitString(log, '|');
    string_view op(sv_vec[0]);
    int cur_epoch = stoi(string(sv_vec[1]));
    sv_vec.erase(sv_vec.begin(), sv_vec.begin() + 1);
    sv_vec.pop_back();
    if (op == "add_vertex") {
      process_add_vertex(sv_vec, graph_stores_[pid]);
    } else if (op == "add_edge") {
      process_add_edge(sv_vec, graph_stores_[pid]);
    } else if (op == "delete_vertex") {
      process_del_vertex(sv_vec, graph_stores_[pid]);
    } else if (op == "delete_edge") {
      process_del_edge(sv_vec, graph_stores_[pid]);
    } else if (op == "update_vertex") {
      process_update_vertex(sv_vec, graph_stores_[pid]);
    } else {
      LOG(ERROR) << "Unsupported operator " << op;
    }
  }
}
#endif

void Runner::apply_log_to_store_(const string_view& log, int p_id) {
#ifndef USE_MULTI_THREADS
  auto sv_vec = splitString(log, '|');
  string_view op(sv_vec[0]);
  int cur_epoch = stoi(string(sv_vec[1]));
#else
  size_t firstDelim = log.find('|');
  size_t secondDelim = log.find('|', firstDelim + 1);
  std::string_view op = log.substr(0, firstDelim);
  int cur_epoch = stoi(
      std::string(log.substr(firstDelim + 1, secondDelim - firstDelim - 1)));
#endif

#ifdef USE_MULTI_THREADS
  if (cur_epoch > latest_epoch_) {
    while (!logs_.empty()) {
      std::this_thread::yield();
    }

    for (auto idx = 0; idx < FLAGS_num_threads; idx++) {
      while (true) {
        working_state_mutex_[idx]->lock_shared();
        bool is_working = is_working_[idx];
        working_state_mutex_[idx]->unlock_shared();
        if (is_working) {
          std::this_thread::yield();
        } else {
          break;
        }
      }
    }

    graph_stores_[p_id]->update_blob(latest_epoch_);
    graph_stores_[p_id]->insert_blob_schema(latest_epoch_);
    // put schema to etcd
    graph_stores_[p_id]->put_blob_json_etcd(latest_epoch_);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        endTime - start_time_)
                        .count();
    start_time_ = endTime;

    cout << "update epoch " << latest_epoch_ << " frag = " << p_id
         << " time = " << duration << " ms using " << fLI::FLAGS_num_threads
         << " threads" << endl;
    latest_epoch_ = cur_epoch;
  }
#else

  if (cur_epoch > latest_epoch_) {
    graph_stores_[p_id]->update_blob(latest_epoch_);
    graph_stores_[p_id]->insert_blob_schema(latest_epoch_);
    // put schema to etcd
    graph_stores_[p_id]->put_blob_json_etcd(latest_epoch_);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        endTime - start_time_)
                        .count();

    start_time_ = endTime;

    cout << "single thread update epoch " << latest_epoch_ << " frag = " << p_id
         << " time = " << duration << " ms" << endl;
    latest_epoch_ = cur_epoch;
  }
#endif

  if (unlikely(op == "bulkload_end")) {
    cout << "Completion of bulkload and transition to epoch " << cur_epoch
         << endl;
    for (int i = 0; i < graph_stores_[p_id]->get_total_vertex_label_num();
         ++i) {
      seggraph::SegGraph* graph =
          graph_stores_[p_id]->get_graph<seggraph::SegGraph>(i);
      size_t usage, limit;
      if (graph) {
        graph->get_v6d_usage(usage, limit);
        printf(
            "frag %d, vlabel %d, inner: %lu vertices, get_block_usage %lu, v6d "
            "usage %.2lf GB, limit %.2lf GB\n",
            p_id, i, graph->get_max_vertex_id(), graph->get_block_usage(),
            static_cast<double>(usage) / (1 << 30),
            static_cast<double>(limit) / (1 << 30));
        fflush(stdout);
      } else {
        LOG(ERROR) << "empty pointer frag " << p_id << ", vlabel " << i;
      }

      seggraph::SegGraph* ovg = graph_stores_[p_id]->get_ov_graph(i);
      if (ovg) {
        ovg->get_v6d_usage(usage, limit);
        printf(
            "frag %d, vlabel %d, outer: %lu vertices, get_block_usage %lu, v6d "
            "usage %.2lf GB, limit %.2lf GB\n",
            p_id, i, ovg->get_max_vertex_id(), ovg->get_block_usage(),
            static_cast<double>(usage) / (1 << 30),
            static_cast<double>(limit) / (1 << 30));
        fflush(stdout);
      }
    }
    return;
  }
#ifndef USE_MULTI_THREADS
  sv_vec.erase(sv_vec.begin(), sv_vec.begin() + 1);
  // remove the last element of sv_vec, since the last element of sv_vec is
  // offset of binlog
  sv_vec.pop_back();
#endif

#ifdef USE_MULTI_THREADS
  if (op == "add_vertex" || op == "add_edge") {
    if (parallel_state_) {
      logs_.push(std::string(log));
    } else {
      while (!logs_.empty()) {
        std::this_thread::yield();
      }

      for (auto idx = 0; idx < FLAGS_num_threads; idx++) {
        while (true) {
          working_state_mutex_[idx]->lock_shared();
          bool is_working = is_working_[idx];
          working_state_mutex_[idx]->unlock_shared();
          if (is_working) {
            std::this_thread::yield();
          } else {
            break;
          }
        }
      }

      logs_.push(std::string(log));
      parallel_state_ = true;
    }
  } else if (op == "delete_vertex" || op == "delete_edge" ||
             op == "update_vertex") {
    parallel_state_ = false;
    while (!logs_.empty()) {
      std::this_thread::yield();
    }

    for (auto idx = 0; idx < FLAGS_num_threads; idx++) {
      while (true) {
        working_state_mutex_[idx]->lock_shared();
        bool is_working = is_working_[idx];
        working_state_mutex_[idx]->unlock_shared();
        if (is_working) {
          std::this_thread::yield();
        } else {
          break;
        }
      }
    }

    logs_.push(std::string(log));
  } else {
    LOG(ERROR) << "Unsupported operator " << op;
  }
  return;
#endif

#ifndef USE_MULTI_THREADS
  if (op == "add_vertex") {
    process_add_vertex(sv_vec, graph_stores_[p_id]);
  } else if (op == "add_edge") {
    process_add_edge(sv_vec, graph_stores_[p_id]);
  } else if (op == "delete_vertex") {
    process_del_vertex(sv_vec, graph_stores_[p_id]);
  } else if (op == "delete_edge") {
    process_del_edge(sv_vec, graph_stores_[p_id]);
  } else if (op == "update_vertex") {
    process_update_vertex(sv_vec, graph_stores_[p_id]);
  } else {
    LOG(ERROR) << "Unsupported operator " << op;
  }
#endif
}

Status Runner::start_kafka_to_process_(int p_id) {
  RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  string rdkafka_err;
  if (conf->set("metadata.broker.list", FLAGS_kafka_broker_list, rdkafka_err) !=
      RdKafka::Conf::CONF_OK) {
    LOG(WARNING) << "Failed to set metadata.broker.list: " << rdkafka_err;
  }
  if (conf->set("group.id", "gart_consumer", rdkafka_err) !=
      RdKafka::Conf::CONF_OK) {
    LOG(WARNING) << "Failed to set group.id: " << rdkafka_err;
  }
  if (conf->set("enable.auto.commit", "false", rdkafka_err) !=
      RdKafka::Conf::CONF_OK) {
    LOG(WARNING) << "Failed to set enable.auto.commit: " << rdkafka_err;
  }
  if (conf->set("auto.offset.reset", "earliest", rdkafka_err) !=
      RdKafka::Conf::CONF_OK) {
    LOG(WARNING) << "Failed to set auto.offset.reset: " << rdkafka_err;
  }

  RdKafka::Consumer* consumer = RdKafka::Consumer::create(conf, rdkafka_err);

  if (!consumer) {
    LOG(INFO) << "Failed to create consumer: " << rdkafka_err;
    return Status::KafkaConnectError();
  }

  RdKafka::Conf* tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

  RdKafka::Topic* topic = RdKafka::Topic::create(
      consumer, FLAGS_kafka_unified_log_topic, tconf, rdkafka_err);
  int32_t partition = 0;
  int64_t start_offset = RdKafka::Topic::OFFSET_BEGINNING;

  RdKafka::ErrorCode resp = consumer->start(topic, partition, start_offset);
  if (resp != RdKafka::ERR_NO_ERROR) {
    LOG(WARNING) << "Failed to start consumer: " << RdKafka::err2str(resp);
    exit(1);
  }

  printf("Start main loop for subgraph %d ...\n", p_id);
  start_time_ = std::chrono::high_resolution_clock::now();
  while (1) {
    RdKafka::Message* msg = consumer->consume(topic, partition, 1000);
    const char* log_base = static_cast<const char*>(msg->payload());
    size_t log_bytes = msg->len();
    if (log_bytes == 0) {
      continue;
    }
    string_view log(log_base, log_bytes);
    apply_log_to_store_(log, p_id);
    delete msg;
  }
}

void Runner::start_file_stream_to_process_(int p_id) {
  ifstream infile(FLAGS_kafka_unified_log_file);
  string line;
  while (getline(infile, line)) {
    apply_log_to_store_(line, p_id);
  }
}

void Runner::load_graph_partitions_from_logs_(int mac_id,
                                              int total_partitions) {
  int num_gp_backups = total_partitions;
  bool load_from_kafka = true;
  graph_stores_.assign(num_gp_backups, nullptr);
  rg_maps_.assign(num_gp_backups, nullptr);
  int p_id = mac_id;

  // load graph
  graph_stores_[p_id] = new graph::GraphStore(
      p_id, gart::framework::config.getServerID(), total_partitions);
  rg_maps_[p_id] = new graph::RGMapping(p_id);
  GART_CHECK_OK(init_graph_schema(FLAGS_etcd_endpoint, FLAGS_meta_prefix,
                                  graph_stores_[p_id], rg_maps_[p_id]));
  graph_stores_[p_id]->put_schema();
  graph_stores_[p_id]->put_schema4gie();
  int v_label_num = graph_stores_[p_id]->get_total_vertex_label_num();
  graph_stores_[p_id]->init_ovg2ls(v_label_num);
  graph_stores_[p_id]->init_vertex_maps(v_label_num);
#ifdef USE_MULTI_THREADS
  std::vector<std::thread> threads;
  for (auto idx = 0; idx < FLAGS_num_threads; idx++) {
    threads.emplace_back(&Runner::process_log_thread, this, p_id, idx);
  }
#endif
#ifndef WITH_TEST
  GART_CHECK_OK(start_kafka_to_process_(p_id));
#else
  start_file_stream_to_process_(p_id);
#endif

#ifdef USE_MULTI_THREADS
  for (auto& thread : threads) {
    thread.join();
  }
#endif
}

void Runner::run() {
  /*************** Load Data ****************/
  int mac_id = gart::framework::config.getServerID();
  int total_partitions = gart::framework::config.getNumServers();
#ifdef USE_MULTI_THREADS
  working_state_mutex_.resize(FLAGS_num_threads);
  for (auto idx = 0; idx < FLAGS_num_threads; idx++) {
    is_working_.push_back(true);
    working_state_mutex_[idx] = std::make_shared<std::shared_timed_mutex>();
  }
#endif

  load_graph_partitions_from_logs_(mac_id, total_partitions);

  printf("[Runner] Complete Loading!\n");
  fflush(stdout);
}

}  // namespace framework
}  // namespace gart
