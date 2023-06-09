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

#include "framework/bench_runner.h"

#include <fstream>

#include "graph/graph_ops.h"
#include "librdkafka/rdkafkacpp.h"

#include "yaml-cpp/yaml.h"

using namespace std;

namespace gart {
namespace framework {

void init_graph_schema(string graph_schema_path, string table_schema_path,
                       graph::GraphStore* graph_store,
                       graph::RGMapping* rg_map) {
  using json = vineyard::json;

  ifstream graph_schema_file(graph_schema_path);
  if (!graph_schema_file.is_open()) {
    LOG(ERROR) << "graph schema file (" << graph_schema_path << ") open failed."
               << "Not exist or permission denied.";
    exit(1);
  }

  ifstream table_schema_file(table_schema_path);
  if (!table_schema_file.is_open()) {
    LOG(ERROR) << "table schema file (" << table_schema_path << ") open failed."
               << "Not exist or permission denied.";
    exit(1);
  }

  YAML::Node config;
  try {
    config = YAML::LoadFile(graph_schema_path);
  } catch (YAML::ParserException& e) {
    LOG(ERROR) << "Parse graph schema file (" << graph_schema_path
               << ") failed: " << e.what();
    exit(1);
  }

  json table_schema;
  try {
    table_schema = json::parse(table_schema_file);
  } catch (json::parse_error& e) {
    LOG(ERROR) << "Parse table schema file (" << table_schema_path
               << ") failed: " << e.what();
    exit(1);
  }

  graph::SchemaImpl graph_schema;
  map<string, int> vertex_name_id_map;

  YAML::Node vdef = config["vertexMappings"]["vertex_types"];
  YAML::Node edef = config["edgeMappings"]["edge_types"];
  assert(vdef.IsSequence() && edef.IsSequence());
  int vlabel_num = vdef.size(), elabel_num = edef.size();

  graph_store->set_vertex_label_num(vlabel_num);
  graph_schema.elabel_offset = vlabel_num;
  int prop_offset = 0;

  Property::Schema prop_schema;          // for vertex properties
  prop_schema.store_type = PROP_COLUMN;  // use column store

  Property::Column col;
  col.page_size = 0;

  // Parse vertex
  for (int idx = 0; idx < vlabel_num; ++idx) {
    int id = idx;
    string name = vdef[idx]["type_name"].as<string>();
    string table_name = vdef[idx]["dataSourceName"].as<string>();
    vertex_name_id_map.emplace(name, id);
    graph_schema.label_id_map[name] = id;
    rg_map->define_vertex(id, id);  // (vlabel, table_id)
    graph_store->add_vgraph(id, rg_map);
    prop_schema.table_id = id;
    prop_schema.klen = sizeof(uint64_t);
  }

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

    graph_schema.label_id_map[name] = id;
    int src_label_id = vertex_name_id_map.find(src_name)->second;
    int dst_label_id = vertex_name_id_map.find(dst_name)->second;
    graph_schema.edge_relation[id] = {src_label_id, dst_label_id};
    // TODO(wanglei): fk and is_directed are hard code
    rg_map->define_nn_edge(id, src_label_id, dst_label_id, 0, 0, false,
                           edef[idx]["dataFieldMappings"].size());
  }

  for (int idx = 0; idx < vlabel_num + elabel_num; ++idx) {
    int id = idx;
    string table_name;
    YAML::Node prop_info;
    bool is_vertex = (idx < vlabel_num);
    if (is_vertex) {
      prop_info = vdef[idx]["mappings"];
      table_name = vdef[idx]["dataSourceName"].as<string>();
    } else {
      prop_info = edef[idx - vlabel_num]["dataFieldMappings"];
      table_name = edef[idx - vlabel_num]["dataSourceName"].as<string>();
    }
    if (prop_info.size() != 0) {
      graph_schema.label2prop_offset[id] = prop_offset;
    }

    uint64_t edge_prop_prefix_bytes = 0;
    for (int prop_idx = 0; prop_idx < prop_info.size(); prop_idx++) {
      int prop_id = prop_idx;
      string prop_name = prop_info[prop_idx]["property"].as<string>();
      string prop_dtype = "";
      string prop_table_col_name =
          prop_info[prop_idx]["dataField"]["name"].as<string>();
      auto required_table_schema = table_schema[table_name];
      for (int col_idx = 0; col_idx < required_table_schema.size(); ++col_idx) {
        if (required_table_schema[col_idx].size() != 2) {
          LOG(ERROR) << "Table schema file (" << table_schema_path
                     << ") format error. "
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
          } else if (prop_dtype_str == "int64") {
            prop_dtype = "LONG";
          } else if (prop_dtype_str == "float") {
            prop_dtype = "FLOAT";
          } else if (prop_dtype_str == "varchar(255)" ||
                     prop_dtype_str == "character varying") {
            prop_dtype = "LONGSTRING";  // TODO: unified string types
          } else if (prop_dtype_str == "text") {
            prop_dtype = "TEXT";
          } else {
            assert(false);
          }
          break;
        }
      }

      if (prop_dtype == "") {
        LOG(ERROR) << "Table schema file (" << table_schema_path
                   << ") format error. "
                   << "Table name " << table_name << " Column name "
                   << prop_table_col_name << " not found.";
        assert(false);
      }

      if (is_vertex) {
        // col.updatable = prop_info[prop_idx]["updatable"].get<bool>();
        col.updatable = true;
      }
      if (prop_dtype == "INT") {
        graph_schema.dtype_map[{id, prop_id}] = INT;
        if (is_vertex) {
          col.vtype = INT;
          col.vlen = sizeof(int);
          prop_schema.cols.push_back(col);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, INT);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(int);
        }
      } else if (prop_dtype == "FLOAT") {
        graph_schema.dtype_map[{id, prop_id}] = FLOAT;
        if (is_vertex) {
          col.vtype = FLOAT;
          col.vlen = sizeof(float);
          prop_schema.cols.push_back(col);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, FLOAT);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(float);
        }
      } else if (prop_dtype == "DOUBLE") {
        graph_schema.dtype_map[{id, prop_id}] = DOUBLE;
        if (is_vertex) {
          col.vtype = DOUBLE;
          col.vlen = sizeof(double);
          prop_schema.cols.push_back(col);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, DOUBLE);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(double);
        }
      } else if (prop_dtype == "LONG") {
        graph_schema.dtype_map[{id, prop_id}] = LONG;
        if (is_vertex) {
          col.vtype = LONG;
          col.vlen = sizeof(uint64_t);
          prop_schema.cols.push_back(col);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, LONG);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(uint64_t);
        }
      } else if (prop_dtype == "CHAR") {
        graph_schema.dtype_map[{id, prop_id}] = CHAR;
        if (is_vertex) {
          col.vtype = CHAR;
          col.vlen = sizeof(char);
          prop_schema.cols.push_back(col);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, CHAR);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(char);
        }
      } else if (prop_dtype == "STRING") {
        graph_schema.dtype_map[{id, prop_id}] = STRING;
        if (is_vertex) {
          col.vtype = STRING;
          col.vlen = sizeof(gart::graph::ldbc::String);
          prop_schema.cols.push_back(col);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, STRING);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(gart::graph::ldbc::String);
        }
      } else if (prop_dtype == "TEXT") {
        graph_schema.dtype_map[{id, prop_id}] = TEXT;
        if (is_vertex) {
          col.vtype = TEXT;
          col.vlen = sizeof(gart::graph::ldbc::Text);
          prop_schema.cols.push_back(col);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, TEXT);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(gart::graph::ldbc::Text);
        }
      } else if (prop_dtype == "DATE") {
        graph_schema.dtype_map[{id, prop_id}] = DATE;
        if (is_vertex) {
          col.vtype = DATE;
          col.vlen = sizeof(gart::graph::ldbc::Date);
          prop_schema.cols.push_back(col);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, DATE);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(gart::graph::ldbc::Date);
        }
      } else if (prop_dtype == "DATETIME") {
        graph_schema.dtype_map[{id, prop_id}] = DATETIME;
        if (is_vertex) {
          col.vtype = DATETIME;
          col.vlen = sizeof(gart::graph::ldbc::DateTime);
          prop_schema.cols.push_back(col);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, DATETIME);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(gart::graph::ldbc::DateTime);
        }
      } else if (prop_dtype == "LONGSTRING") {
        graph_schema.dtype_map[{id, prop_id}] = LONGSTRING;
        if (is_vertex) {
          col.vtype = LONGSTRING;
          col.vlen = sizeof(gart::graph::ldbc::LongString);
          prop_schema.cols.push_back(col);
        } else {
          graph_store->insert_edge_property_dtypes(id, prop_id, LONGSTRING);
          graph_store->insert_edge_prop_prefix_bytes(id, prop_id,
                                                     edge_prop_prefix_bytes);
          edge_prop_prefix_bytes += sizeof(gart::graph::ldbc::LongString);
        }
      } else {
        assert(false);
      }

      graph_schema.property_id_map[std::make_pair(prop_name, idx)] =
          prop_offset;
      prop_offset++;
    }
    if (is_vertex) {
      graph_store->add_vprop(id, prop_schema);
      prop_schema.cols.clear();
    } else {
      graph_store->insert_edge_prop_total_bytes(id, edge_prop_prefix_bytes);
      edge_prop_prefix_bytes = 0;
    }
  }

  graph_store->set_schema(graph_schema);
  graph_store->update_property_bytes();
}

void Runner::apply_log_to_store_(string log, int p_id) {
  istringstream strstr(log);
  string word;
  vector<string> cmd;
  int word_idx = 0;
  string op;
  while (getline(strstr, word, '|')) {
    if (word_idx == 0) {
      op = word;
    } else {
      if (word_idx == 1) {
        int cur_epoch = stoi(word);
        if (cur_epoch > latest_epoch_) {
          graph_stores_[p_id]->update_blob(latest_epoch_);
          graph_stores_[p_id]->insert_blob_schema(latest_epoch_);
          graph_stores_[p_id]->get_blob_json(
              latest_epoch_);  // put schema to etcd
          cout << "update epoch " << latest_epoch_ << " frag = " << p_id
               << endl;
          latest_epoch_ = cur_epoch;
        }
      }
      cmd.push_back(word);
    }
    word_idx++;
  }

  // TODO: till now, update = delete + insert
  if (op == "add_vertex") {
    process_add_vertex(cmd, graph_stores_[p_id]);
  } else if (op == "add_edge") {
    process_add_edge(cmd, graph_stores_[p_id]);
  } else if (op == "delete_vertex") {
    process_del_vertex(cmd, graph_stores_[p_id]);
  } else if (op == "delete_edge") {
    process_del_edge(cmd, graph_stores_[p_id]);
  } else if (op == "update_vertex") {
    process_del_vertex(cmd, graph_stores_[p_id]);
    process_add_vertex(cmd, graph_stores_[p_id]);
  } else if (op == "update_edge") {
    process_del_edge(cmd, graph_stores_[p_id]);
    process_add_edge(cmd, graph_stores_[p_id]);
  } else {
    LOG(ERROR) << "Unsupported operator " << op;
  }
}

void Runner::start_kafka_to_process_(int p_id) {
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
    exit(1);
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
  while (1) {
    RdKafka::Message* msg = consumer->consume(topic, partition, 1000);
    string str;
    const char* str_addr = static_cast<const char*>(msg->payload());
    int str_len = static_cast<int>(msg->len());
    if (str_len == 0) {
      continue;
    }
    str.assign(str_addr, str_addr + str_len);
    apply_log_to_store_(str, p_id);
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
  init_graph_schema(FLAGS_schema_file_path, FLAGS_table_schema_file_path,
                    graph_stores_[p_id], rg_maps_[p_id]);
  graph_stores_[p_id]->put_schema();
  int v_label_num = graph_stores_[p_id]->get_total_vertex_label_num();
  graph_stores_[p_id]->init_ovg2ls(v_label_num);
#ifndef WITH_TEST
  start_kafka_to_process_(p_id);
#else
  start_file_stream_to_process_(p_id);
#endif
}

void Runner::run() {
  /*************** Load Data ****************/
  int mac_id = gart::framework::config.getServerID();
  int total_partitions = gart::framework::config.getNumServers();

  load_graph_partitions_from_logs_(mac_id, total_partitions);

  printf("[Runner] Complete Loading!\n");
  fflush(stdout);
}

}  // namespace framework
}  // namespace gart
