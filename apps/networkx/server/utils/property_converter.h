/** Copyright 2020 Alibaba Group Holding Limited.
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

#ifndef SERVER_UTILS_PROPERTY_CONVERTER_H_
#define SERVER_UTILS_PROPERTY_CONVERTER_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "server/utils/dynamic.h"

namespace gart {

template <typename FRAGMENT_T>
struct PropertyConverter {
  inline static void NodeValue(
      const std::shared_ptr<FRAGMENT_T>& fragment,
      const typename FRAGMENT_T::vertex_t& v, const std::string data_type,
      const std::string& prop_name, int prop_id, rapidjson::Value& ret,
      dynamic::AllocatorT& allocator = dynamic::Value::allocator_) {
    if (data_type == "INT") {
      rapidjson::Value value(fragment->template GetData<int32_t>(v, prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "FLOAT") {
      rapidjson::Value value(fragment->template GetData<float>(v, prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "DOUBLE") {
      rapidjson::Value value(fragment->template GetData<double>(v, prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "LONG") {
      rapidjson::Value value(fragment->template GetData<uint64_t>(v, prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "STRING") {
      rapidjson::Value value(
          fragment->template GetData<std::string_view>(v, prop_id).data(),
          allocator);
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "DATE") {
      rapidjson::Value value(fragment->template GetData<int>(v, prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "DATETIME") {
      rapidjson::Value value(fragment->template GetData<uint64_t>(v, prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "TIME") {
      rapidjson::Value value(fragment->template GetData<int>(v, prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "TIMESTAMP") {
      rapidjson::Value value(fragment->template GetData<uint64_t>(v, prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else {
      std::cout << "Unknown data type: " << data_type << std::endl;
    }
  }

  inline static void EdgeValue(
      const std::shared_ptr<FRAGMENT_T>& fragment,
      gart::EdgeIterator& edge_iter, const std::string data_type,
      const std::string& prop_name, int prop_id, rapidjson::Value& ret,
      dynamic::AllocatorT& allocator = dynamic::Value::allocator_) {
    if (data_type == "INT") {
      rapidjson::Value value(edge_iter.template get_data<int>(prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "FLOAT") {
      rapidjson::Value value(edge_iter.template get_data<float>(prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "DOUBLE") {
      rapidjson::Value value(edge_iter.template get_data<double>(prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "LONG") {
      rapidjson::Value value(edge_iter.template get_data<uint64_t>(prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "STRING") {
      rapidjson::Value value(
          edge_iter.template get_data<std::string_view>(prop_id).data(),
          allocator);
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "DATE") {
      rapidjson::Value value(edge_iter.template get_data<int>(prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "DATETIME") {
      rapidjson::Value value(edge_iter.template get_data<uint64_t>(prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "TIME") {
      rapidjson::Value value(edge_iter.template get_data<int>(prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else if (data_type == "TIMESTAMP") {
      rapidjson::Value value(edge_iter.template get_data<uint64_t>(prop_id));
      ret.AddMember(rapidjson::Value(prop_name, allocator).Move(), value,
                    allocator);
    } else {
      std::cout << "Unknown data type: " << data_type << std::endl;
    }
  }
};
}  // namespace gart
#endif  // SERVER_UTILS_PROPERTY_CONVERTER_H_