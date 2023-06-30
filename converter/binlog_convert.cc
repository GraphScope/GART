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

#include <fstream>
#include <map>

#include <gflags/gflags.h>

#include "flags.h"           // NOLINT(build/include_subdir)
#include "kafka_producer.h"  // NOLINT(build/include_subdir)
#include "vegito/src/fragment/id_parser.h"
#include "vineyard/common/util/json.h"
#include "yaml-cpp/yaml.h"

using json = vineyard::json;
using namespace std;

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

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // init kafka consumer and producer
  RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  string rdkafka_err;
  if (conf->set("metadata.broker.list", FLAGS_read_kafka_broker_list,
                rdkafka_err) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "Failed to set metadata.broker.list: " << rdkafka_err;
  }
  if (conf->set("group.id", "gart_consumer", rdkafka_err) !=
      RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "Failed to set group.id: " << rdkafka_err;
  }
  if (conf->set("enable.auto.commit", "false", rdkafka_err) !=
      RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "Failed to set enable.auto.commit: " << rdkafka_err;
  }
  if (conf->set("auto.offset.reset", "earliest", rdkafka_err) !=
      RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "Failed to set auto.offset.reset: " << rdkafka_err;
  }

  RdKafka::Consumer* consumer = RdKafka::Consumer::create(conf, rdkafka_err);
  if (!consumer) {
    LOG(ERROR) << "Failed to create consumer: " << rdkafka_err << endl;
    exit(1);
  }

  RdKafka::Conf* tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

  RdKafka::Topic* topic = RdKafka::Topic::create(
      consumer, fLS::FLAGS_read_kafka_topic, tconf, rdkafka_err);
  int32_t partition = 0;
  int64_t start_offset = RdKafka::Topic::OFFSET_BEGINNING;

  RdKafka::ErrorCode resp = consumer->start(topic, partition, start_offset);

  shared_ptr<KafkaProducer> producer = make_shared<KafkaProducer>(
      fLS::FLAGS_write_kafka_broker_list, fLS::FLAGS_write_kafka_topic);
  KafkaOutputStream ostream(producer);

  if (resp != RdKafka::ERR_NO_ERROR) {
    LOG(ERROR) << "Failed to start consumer: " << RdKafka::err2str(resp);
    exit(1);
  }

  // init graph schema
  map<string, int> table2vlabel;  // data source (table) -> vlabel id (from 0)
  map<string, int> table2elabel;  // data source (table) -> elabel id (from 0)
  map<string, vector<string>> required_properties;
  map<string, string> vertex_label_columns;  // table name -> id column name
  // table name -> (src column name, dst column name)
  map<string, pair<string, string>> edge_label_columns;
  vector<pair<int, int>> edge_label2src_dst_labels;
  map<string, int> vertex_label2ids;

  ifstream rg_mapping_file_stream(FLAGS_rg_mapping_file_path);
  if (!rg_mapping_file_stream.is_open()) {
    LOG(ERROR) << "RGMapping file (" << FLAGS_rg_mapping_file_path
               << ") open failed."
               << "Not exist or permission denied.";
    exit(1);
  }

  YAML::Node rg_mapping;
  try {
    rg_mapping = YAML::LoadFile(FLAGS_rg_mapping_file_path);
  } catch (YAML::ParserException& e) {
    LOG(ERROR) << "RGMapping file (" << FLAGS_rg_mapping_file_path
               << ") parse failed."
               << "Error message: " << e.what();
    exit(1);
  }
  YAML::Node vdef = rg_mapping["vertexMappings"]["vertex_types"];
  YAML::Node edef = rg_mapping["edgeMappings"]["edge_types"];
  assert(vdef.IsSequence() && edef.IsSequence());
  int vlabel_num = vdef.size(), elabel_num = edef.size();
  gart::IdParser<int64_t> id_parser;
  id_parser.Init(FLAGS_subgraph_num, vlabel_num);

  // parse vertex
  for (int idx = 0; idx < vlabel_num; ++idx) {
    auto id = idx;
    auto table_name = vdef[idx]["dataSourceName"].as<string>();
    auto label = vdef[idx]["type_name"].as<string>();
    table2vlabel.emplace(table_name, id);
    vertex_label_columns.emplace(table_name,
                                 vdef[idx]["idFieldName"].as<string>());
    vertex_label2ids.emplace(label, id);
    YAML::Node properties = vdef[idx]["mappings"];
    vector<string> required_prop_names;
    for (uint64_t prop_id = 0; prop_id < properties.size(); prop_id++) {
      auto prop_name = properties[prop_id]["dataField"]["name"].as<string>();
      required_prop_names.emplace_back(prop_name);
    }
    required_properties.emplace(table_name, required_prop_names);
  }

  // parse edges
  for (int idx = 0; idx < elabel_num; ++idx) {
    auto table_name = edef[idx]["dataSourceName"].as<string>();
    table2elabel.emplace(table_name, idx);
    auto src_label = edef[idx]["type_pair"]["source_vertex"].as<string>();
    auto dst_label = edef[idx]["type_pair"]["destination_vertex"].as<string>();
    auto src_col =
        edef[idx]["sourceVertexMappings"][0]["dataField"]["name"].as<string>();
    auto dst_col =
        edef[idx]["destinationVertexMappings"][0]["dataField"]["name"]
            .as<string>();
    edge_label_columns.emplace(table_name, make_pair(src_col, dst_col));
    auto src_label_id = vertex_label2ids.find(src_label)->second;
    auto dst_label_id = vertex_label2ids.find(dst_label)->second;
    edge_label2src_dst_labels.push_back(make_pair(src_label_id, dst_label_id));
    YAML::Node properties = edef[idx]["dataFieldMappings"];
    vector<string> required_prop_names;
    for (uint64_t prop_id = 0; prop_id < properties.size(); prop_id++) {
      auto prop_name = properties[prop_id]["dataField"]["name"].as<string>();
      required_prop_names.emplace_back(prop_name);
    }
    required_properties.emplace(table_name, required_prop_names);
  }

  // start to process log
  vector<map<string, int64_t>> string_oid2gid_maps(vlabel_num);
  vector<map<int64_t, int64_t>> int64_oid2gid_maps(vlabel_num);
  vector<uint64_t> vertex_nums(vlabel_num, 0);
  vector<vector<uint64_t>> vertex_nums_per_fragment(vlabel_num);

  for (auto idx = 0; idx < vlabel_num; ++idx) {
    vertex_nums_per_fragment[idx].resize(FLAGS_subgraph_num, 0);
  }

  int log_count = 0;
  while (1) {
    RdKafka::Message* msg = consumer->consume(topic, partition, 1000);
    int str_len = static_cast<int>(msg->len());

    // skip empty message to avoid JSON parser error
    if (str_len == 0) {
      continue;
    }

    const char* str_addr = static_cast<const char*>(msg->payload());
    string line;
    line.assign(str_addr, str_addr + str_len);
    json log = json::parse(line);

    bool is_edge = false;
    string type = log["type"].get<string>();
    string table_name = log["table"].get<string>();
    if (table2vlabel.find(table_name) == table2vlabel.end() &&
        table2elabel.find(table_name) == table2elabel.end()) {
      continue;
    }

    if (table2elabel.find(table_name) != table2elabel.end()) {
      is_edge = true;
      if (table2vlabel.find(table_name) != table2vlabel.end()) {
        LOG(ERROR) << "Table name conflict: " << table_name;
        assert(false);
      }
    }

    string content;
    int epoch = log_count / FLAGS_logs_per_epoch;
    auto data = log["data"];

    if (type == "insert") {
      content = is_edge ? "add_edge" : "add_vertex";
      append_str(content, epoch);

      if (!is_edge) {
        auto vid_col = vertex_label_columns.find(table_name)->second;
        auto vertex_label_id = table2vlabel.find(table_name)->second;
        int64_t fid = vertex_nums[vertex_label_id] % FLAGS_subgraph_num;
        int64_t offset = vertex_nums_per_fragment[vertex_label_id][fid];
        vertex_nums[vertex_label_id]++;
        vertex_nums_per_fragment[vertex_label_id][fid]++;
        auto gid = id_parser.GenerateId(fid, vertex_label_id, offset);
        append_str(content, gid);
        if (data[vid_col].is_number_integer()) {
          int64_oid2gid_maps[vertex_label_id].emplace(
              data[vid_col].get<int64_t>(), gid);
        } else if (data[vid_col].is_string()) {
          string_oid2gid_maps[vertex_label_id].emplace(
              data[vid_col].get<string>(), gid);
        }
      } else {
        append_str(content, table2elabel[table_name]);

        auto edge_col = edge_label_columns.find(table_name)->second;
        string src_name = edge_col.first;
        string dst_name = edge_col.second;
        auto edge_label_id = table2elabel.find(table_name)->second;
        int src_label_id = edge_label2src_dst_labels[edge_label_id].first;
        int dst_label_id = edge_label2src_dst_labels[edge_label_id].second;
        string src_vid, dst_vid;
        if (data[src_name].is_number_integer()) {
          src_vid = to_string(int64_oid2gid_maps[src_label_id]
                                  .find(data[src_name].get<int64_t>())
                                  ->second);
        } else if (data[src_name].is_string()) {
          src_vid = to_string(string_oid2gid_maps[src_label_id]
                                  .find(data[src_name].get<string>())
                                  ->second);
        }
        if (data[dst_name].is_number_integer()) {
          dst_vid = to_string(int64_oid2gid_maps[dst_label_id]
                                  .find(data[dst_name].get<int64_t>())
                                  ->second);
        } else if (data[dst_name].is_string()) {
          dst_vid = to_string(string_oid2gid_maps[dst_label_id]
                                  .find(data[dst_name].get<string>())
                                  ->second);
        }
        append_str(content, src_vid);
        append_str(content, dst_vid);
      }

      auto iter = required_properties.find(table_name);
      auto required_prop_names = iter->second;
      for (size_t prop_id = 0; prop_id < required_prop_names.size();
           prop_id++) {
        auto prop_name = required_prop_names[prop_id];
        auto prop_value = data[prop_name];
        string prop_str;
        if (prop_value.is_string()) {
          prop_str = prop_value.get<string>();
        } else if (prop_value.is_number_integer()) {
          prop_str = to_string(prop_value.get<int>());
        } else if (prop_value.is_number_float()) {
          prop_str = to_string(prop_value.get<float>());
        } else {
          LOG(ERROR) << "Unsupported property type: " << prop_value.type_name();
          assert(false);
          continue;
        }
        append_str(content, prop_str);
      }
    } else if (type == "delete") {
      // TODO(wanglei): add delete vertex and edge support
      content = is_edge ? "del_edge" : "del_vertex";
      append_str(content, epoch);

      if (!is_edge) {
        auto vid_col = vertex_label_columns.find(table_name)->second;
        auto vertex_label_id = table2vlabel.find(table_name)->second;
        int64_t fid = vertex_nums[vertex_label_id] % FLAGS_subgraph_num;
        vertex_nums[vertex_label_id]++;
        vertex_nums_per_fragment[vertex_label_id][fid]++;
        long gid;
        if (data[vid_col].is_number_integer()) {
          gid =
              int64_oid2gid_maps[vertex_label_id][data[vid_col].get<int64_t>()];
        } else if (data[vid_col].is_string()) {
          gid =
              string_oid2gid_maps[vertex_label_id][data[vid_col].get<string>()];
        }
        append_str(content, gid);

      } else {
        append_str(content, table2elabel[table_name]);

        auto edge_col = edge_label_columns.find(table_name)->second;
        string src_name = edge_col.first;
        string dst_name = edge_col.second;
        auto edge_label_id = table2elabel.find(table_name)->second;
        int src_label_id = edge_label2src_dst_labels[edge_label_id].first;
        int dst_label_id = edge_label2src_dst_labels[edge_label_id].second;
        string src_vid, dst_vid;
        if (data[src_name].is_number_integer()) {
          src_vid = to_string(int64_oid2gid_maps[src_label_id]
                                  .find(data[src_name].get<int64_t>())
                                  ->second);
        } else if (data[src_name].is_string()) {
          src_vid = to_string(string_oid2gid_maps[src_label_id]
                                  .find(data[src_name].get<string>())
                                  ->second);
        }
        if (data[dst_name].is_number_integer()) {
          dst_vid = to_string(int64_oid2gid_maps[dst_label_id]
                                  .find(data[dst_name].get<int64_t>())
                                  ->second);
        } else if (data[dst_name].is_string()) {
          dst_vid = to_string(string_oid2gid_maps[dst_label_id]
                                  .find(data[dst_name].get<string>())
                                  ->second);
        }
        append_str(content, src_vid);
        append_str(content, dst_vid);
      }

    } else if (type == "update") {
      // TODO(wanglei): add update vertex and edge support
      content = is_edge ? "up_edge" : "up_vertex";
      append_str(content, epoch);

    } else {
      continue;
    }

    stringstream ss;
    ss << content;
    ostream << ss.str() << flush;
    log_count++;
  }
}
