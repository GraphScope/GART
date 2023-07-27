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

#ifndef CONVERTER_PARSER_H_
#define CONVERTER_PARSER_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "vineyard/common/util/json.h"

#include "vegito/src/fragment/id_parser.h"

namespace converter {

class LogEntry {
 public:
  static LogEntry bulk_load_end();

  std::string to_string() const;

  bool valid() const { return valid_; }

  bool last_snapshot() const { return snapshot == Snapshot::LAST; }

  bool update_has_finish_delete;

 private:
  enum class EntityType { VERTEX, EDGE };
  enum class OpType { INSERT, UPDATE, DELETE, BULKLOAD_END, UNKNOWN };
  enum class Snapshot { FALSE, TRUE, LAST, OTHER };

  // log content
  EntityType entity_type;
  OpType op_type;
  int epoch;
  union {
    struct {
      uint64_t gid;
    } vertex;
    struct {
      int elabel;
      int64_t src_gid;
      int64_t dst_gid;
    } edge;
  };
  std::vector<std::string> properties;

  // log status (meta-data)
  bool valid_;
  Snapshot snapshot;

  friend class TxnLogParser;
};

class TxnLogParser {
 public:
  TxnLogParser(const std::string& rgmapping_file, int subgraph_num) {
    init(rgmapping_file, subgraph_num);
  }

  void parse(LogEntry& out, const std::string& log_str, int epoch);

  ~TxnLogParser() = default;

 private:
  TxnLogParser() = default;

  int64_t get_gid(const vineyard::json& oid, int vlabel) const;

  void set_gid(const vineyard::json& oid, int vlabel, int64_t gid);

  void init(const std::string& rgmapping_file, int subgraph_num);

  void fill_vertex(LogEntry& out, const vineyard::json& log);

  void fill_edge(LogEntry& out, const vineyard::json& log) const;

  void fill_prop(LogEntry& out, const vineyard::json& log) const;

  // used for schema mapping (unchanged after init)
  int vlabel_num_;
  int subgraph_num_;
  std::map<std::string, int>
      table2vlabel_;  // data source (table) -> vlabel id (from 0)
  std::map<std::string, int>
      table2elabel_;  // data source (table) -> elabel id (from 0)
  std::map<std::string, std::vector<std::string>>
      required_properties_;  // table name -> required column names
  std::map<std::string, std::string>
      vertex_id_columns_;  // table name -> id column name
  std::map<std::string, std::pair<std::string, std::string>>
      edge_label_columns_;  // table name -> (src column name, dst column
                            // name)
  std::vector<std::pair<int, int>> edge_label2src_dst_labels_;
  gart::IdParser<int64_t> id_parser_;

  // used for log parsing
  std::vector<std::map<std::string, int64_t>> string_oid2gid_maps_;
  std::vector<std::map<int64_t, int64_t>> int64_oid2gid_maps_;
  std::vector<uint64_t> vertex_nums_;
  std::vector<std::vector<uint64_t>> vertex_nums_per_fragment_;
};

}  // namespace converter

#endif  // CONVERTER_PARSER_H_
