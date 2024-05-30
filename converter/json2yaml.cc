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
#include <iostream>
#include <iterator>
#include <string>

#include "vineyard/common/util/json.h"
#include "yaml-cpp/emittermanip.h"
#include "yaml-cpp/yaml.h"

using std::cerr;
using std::endl;
using std::string;
using vineyard::json;

void parse_json(const string& json_file, const string& yaml_file) {
  // check if the file exists and can be parsed
  std::ifstream json_if(json_file);
  if (!json_if.is_open()) {
    cerr << "RGMapping file (" << json_file << ") open failed."
         << "Not exist or permission denied." << endl;
    exit(1);
  }

  std::ofstream yaml_of(yaml_file);
  if (!yaml_of.is_open()) {
    cerr << "RGMapping file (" << yaml_file << ") open failed."
         << "Not exist or permission denied." << endl;
    exit(1);
  }

  json rg_mapping;
  try {
    rg_mapping = json::parse(json_if);
  } catch (json::parse_error& e) {
    cerr << "RGMapping file (" << json_file << ") parse failed."
         << "Error message: " << e.what() << endl;
    exit(1);
  }

  json::array_t types = rg_mapping["types"];
  int vertex_label_num = rg_mapping["vertexLabelNum"].get<int>();

  YAML::Emitter out;
  out << YAML::BeginDoc << YAML::BeginMap;

  // YAML header
  out << YAML::Key << "graph" << YAML::Value << "LDBC";
  out << YAML::Key << "loadingConfig" << YAML::Value;
  {
    out << YAML::BeginMap;
    out << YAML::Key << "dataSource" << YAML::Value << "rdbms";
    out << YAML::Key << "database" << YAML::Value << "ldbc";
    out << YAML::Key << "method" << YAML::Value << "append";
    out << YAML::EndMap;
  }

  // Vertex types
  out << YAML::Key << "vertexMappings";
  out << YAML::Value << YAML::BeginMap << YAML::Key << "vertex_types";
  out << YAML::Value << YAML::BeginSeq;
  for (int idx = 0; idx < vertex_label_num; ++idx) {
    out << YAML::BeginMap;

    auto type = types[idx]["type"].get<std::string>();
    assert(type == "VERTEX");
    auto label = types[idx]["label"].get<std::string>();
    auto table_name = types[idx]["table_name"].get<std::string>();
    auto id_col = types[idx]["id_column_name"].get<std::string>();
    json::array_t properties = types[idx]["propertyDefList"];

    out << YAML::Key << "type_name" << YAML::Value << label;
    out << YAML::Key << "dataSourceName" << YAML::Value << table_name;
    out << YAML::Key << "idFieldName" << YAML::Value << id_col;

    out << YAML::Key << "mappings" << YAML::Value << YAML::BeginSeq;
    for (uint64_t prop_id = 0; prop_id < properties.size(); prop_id++) {
      out << YAML::BeginMap;
      auto prop_name = properties[prop_id]["name"].get<std::string>();
      auto col_name = properties[prop_id]["column_name"].get<std::string>();

      out << YAML::Key << "property" << YAML::Value << prop_name;
      out << YAML::Key << "dataField" << YAML::Value << YAML::BeginMap;
      out << YAML::Key << "name" << YAML::Value << col_name << YAML::EndMap;

      out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;
  }
  out << YAML::EndSeq << YAML::EndMap;

  // Edge types
  out << YAML::Key << "edgeMappings";
  out << YAML::Value << YAML::BeginMap << YAML::Key << "edge_types";
  out << YAML::Value << YAML::BeginSeq;
  for (size_t idx = vertex_label_num; idx < types.size(); ++idx) {
    out << YAML::BeginMap;

    auto type = types[idx]["type"].get<std::string>();
    assert(type == "EDGE");
    auto label = types[idx]["label"].get<std::string>();
    auto src_label = types[idx]["rawRelationShips"]
                         .at(0)["srcVertexLabel"]
                         .get<std::string>();
    auto dst_label = types[idx]["rawRelationShips"]
                         .at(0)["dstVertexLabel"]
                         .get<std::string>();
    auto table_name = types[idx]["table_name"].get<std::string>();
    auto src_col = types[idx]["rawRelationShips"]
                       .at(0)["src_column_name"]
                       .get<std::string>();
    auto dst_col = types[idx]["rawRelationShips"]
                       .at(0)["dst_column_name"]
                       .get<std::string>();
    json::array_t properties = types[idx]["propertyDefList"];

    out << YAML::Key << "type_pair" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "edge" << YAML::Value << label;
    out << YAML::Key << "source_vertex" << YAML::Value << src_label;
    out << YAML::Key << "destination_vertex" << YAML::Value << dst_label
        << YAML::EndMap;
    out << YAML::Key << "dataSourceName" << YAML::Value << table_name;

    out << YAML::Key << "sourceVertexMappings" << YAML::Value << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << YAML::Key << "dataField" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << src_col << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::EndSeq;

    out << YAML::Key << "destinationVertexMappings" << YAML::Value
        << YAML::BeginSeq;
    out << YAML::BeginMap;
    out << YAML::Key << "dataField" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << dst_col << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::EndSeq;

    out << YAML::Key << "dataFieldMappings" << YAML::Value << YAML::BeginSeq;
    for (uint64_t prop_id = 0; prop_id < properties.size(); prop_id++) {
      out << YAML::BeginMap;
      auto prop_name = properties[prop_id]["name"].get<std::string>();
      auto col_name = properties[prop_id]["column_name"].get<std::string>();

      out << YAML::Key << "property" << YAML::Value << prop_name;
      out << YAML::Key << "dataField" << YAML::Value << YAML::BeginMap;
      out << YAML::Key << "name" << YAML::Value << col_name << YAML::EndMap;

      out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;
  }
  out << YAML::EndSeq << YAML::EndMap;

  out << YAML::EndMap << YAML::EndDoc;
  yaml_of << out.c_str() << endl;

  json_if.close();
  yaml_of.close();
}

int main(int argc, char* argv[]) {
  string json_file = "schema/rgmapping-ldbc.json";
  string yaml_file = "schema/rgmapping-ldbc.yaml";

  if (argc == 3) {
    json_file = argv[1];
    yaml_file = argv[2];
  }

  parse_json(json_file, yaml_file);
  return 0;
}
