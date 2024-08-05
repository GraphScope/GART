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
#include <set>
#include <string>
#include <utility>
#include <vector>
#ifdef ENABLE_CHECKPOINT
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#endif  // ENABLE_CHECKPOINT

#ifdef USE_FLAT_HASH_MAP
#include "absl/container/flat_hash_map.h"
#endif

#include "vineyard/common/util/json.h"

#include "vegito/include/fragment/id_parser.h"
#include "vegito/include/util/status.h"

namespace converter {

class LogEntry {
 public:
  LogEntry()
      : valid_(false),
        update_has_finish_delete(false),
        all_labels_have_process(false),
        complete_(true),
        table_idx(0) {}

  enum class EntityType { VERTEX, EDGE };
  enum class OpType { INSERT, UPDATE, DELETE, BULKLOAD_END, UNKNOWN };
  enum class Snapshot { FALSE, TRUE, LAST, OTHER };

  static LogEntry bulk_load_end();

  std::string to_string(int64_t binlog_offset) const;

  int get_tx_id() const { return tx_id; }

  bool valid() const { return valid_; }

  bool complete() const { return complete_; }

  bool last_snapshot() const { return snapshot == Snapshot::LAST; }

  EntityType get_entity_type() const { return entity_type; }

  OpType get_op_type() const { return op_type; }

  // One log may produce multiple log entries
  // Now it is only used for update edges (delete + insert)
  bool more_entires() const {
    return update_has_finish_delete || !all_labels_have_process;
  }

 private:
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

  // vertex/edge id in original data source
  std::string external_id;
  std::string src_external_id;
  std::string dst_external_id;

  int vlabel;
  int src_vlabel, dst_vlabel;
  bool is_string_oid;
  bool is_src_string_oid, is_dst_string_oid;

  std::vector<std::string> properties;

  // log status (meta-data)
  bool valid_;
  bool update_has_finish_delete;
  bool all_labels_have_process;
  bool complete_;  // incomplete log entry (column maybe null)
  int tx_id;
  size_t table_idx;
  Snapshot snapshot;

  friend class TxnLogParser;
};

class TxnLogParser {
 public:
  TxnLogParser(const std::string& etcd_endpoint, const std::string& etcd_prefix,
               int subgraph_num) {
    GART_CHECK_OK(init(etcd_endpoint, etcd_prefix, subgraph_num));
  }

  gart::Status parse(LogEntry& out, const std::string& log_str);

  gart::Status parse_again(LogEntry& out, int epoch);

#ifdef ENABLE_CHECKPOINT
  void checkpoint_vertex_maps(const std::string& folder_path);

  void load_vertex_maps_checkpoint(const std::string& folder_path);
#endif  // ENABLE_CHECKPOINT

  ~TxnLogParser() = default;

 private:
  TxnLogParser() = default;

  int64_t get_gid(std::string& oid, bool is_string_oid, int vlabel) const;

  void set_gid(std::string& oid, bool is_string_oid, int vlabel, int64_t gid);

  gart::Status init(const std::string& etcd_endpoint,
                    const std::string& etcd_prefix, int subgraph_num);

  void fill_vertex(LogEntry& out, const vineyard::json& log);

  void fill_edge(LogEntry& out, const vineyard::json& log) const;

  void fill_prop(LogEntry& out, const vineyard::json& log) const;

  // used for schema mapping (unchanged after init)
  int vlabel_num_;
  int subgraph_num_;
  std::set<std::string> useful_tables_;
  std::set<std::string> unused_tables_;
  std::set<std::string> vlable_names_;
  // table_name -> vertex_label names (one table may responses to multiple
  // vertex labels)
  std::map<std::string, std::vector<std::string>> table2label_names_;
  std::map<std::string, int>
      vertex_label2ids_;  // vlabel name -> vlabel id (from 0)
  std::map<std::string, int> elabel_names2elabel_;
  std::map<std::string, std::vector<std::string>>
      required_properties_;  // vertex/edge label names -> required column names
  std::map<std::string, std::string>
      vertex_id_columns_;  // vertex_label name -> id column name
  std::map<std::string, std::pair<std::string, std::string>>
      edge_label_columns_;  // edge_label name -> (src column name, dst column
                            // name)
  std::vector<std::pair<int, int>> edge_label2src_dst_labels_;
  gart::IdParser<int64_t> id_parser_;

// used for log parsing
#ifndef USE_FLAT_HASH_MAP
  std::vector<std::map<std::string, int64_t>> string_oid2gid_maps_;
  std::vector<std::map<int64_t, int64_t>> int64_oid2gid_maps_;
#else
  std::vector<absl::flat_hash_map<std::string, int64_t>> string_oid2gid_maps_;
  std::vector<absl::flat_hash_map<int64_t, int64_t>> int64_oid2gid_maps_;
#endif
  std::vector<uint64_t> vertex_nums_;
  std::vector<std::vector<uint64_t>> vertex_nums_per_fragment_;
};

}  // namespace converter

#endif  // CONVERTER_PARSER_H_
