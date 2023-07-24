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

#include <cassert>
#include "graph/graph_ops.h"

using namespace std;

namespace {

template <typename T>
inline void assign(void* ptr, T val) {
  *reinterpret_cast<T*>(ptr) = val;
}

}  // namespace

namespace gart {
namespace graph {

void assign_prop(int data_type, void* prop_ptr, graph::GraphStore* graph_store,
                 const std::string& val) {
  try {
    switch (data_type) {
    case CHAR:
      assign(prop_ptr, val.at(0));
      break;
    case SHORT:
      assign(prop_ptr, short(std::stoi(val)));
      break;
    case INT:
      assign(prop_ptr, std::stoi(val));
      break;
    case LONG:
      assign(prop_ptr, std::stoll(val));
      break;
    case FLOAT:
      assign(prop_ptr, std::stof(val));
      break;
    case DOUBLE:
      assign(prop_ptr, std::stod(val));
      break;
    case STRING: {
      auto str_len = val.length();
      size_t old_offset = graph_store->get_string_buffer_offset();
      char* string_buffer = graph_store->get_string_buffer();
      size_t new_offset = old_offset + str_len;
      assert(new_offset < graph_store->get_string_buffer_size());
      memcpy(string_buffer + old_offset, val.c_str(), str_len);
      graph_store->set_string_buffer_offset(new_offset);
      size_t value = (old_offset << 16) | str_len;
      assign(prop_ptr, value);
      break;
    }
    case DATE:
      assign(prop_ptr, ldbc::Date(val));
      break;
    case DATETIME:
      assign(prop_ptr, ldbc::DateTime(val));
      break;
    default:
      LOG(ERROR) << "Unsupported data type: " << data_type;
    }
  } catch (std::exception& e) {
    LOG(ERROR) << "Failed to assign property: " << e.what()
               << ", data type: " << data_type << ", value: " << val;
  }
}

void process_add_vertex(const StringViewList& cmd,
                        graph::GraphStore* graph_store) {
  int write_epoch = stoi(string(cmd[0]));
  uint64_t vid = static_cast<uint64_t>(stoll(string(cmd[1])));
  StringViewList props(cmd.begin() + 2, cmd.end());
  graph_store->insert_inner_vertex(write_epoch, vid, props);
}

}  // namespace graph
}  // namespace gart
