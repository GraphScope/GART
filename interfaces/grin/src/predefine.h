/** Copyright 2020 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef RESEARCH_GART_GRIN_SRC_PREDEFINE_H_
#define RESEARCH_GART_GRIN_SRC_PREDEFINE_H_

#include <map>
#include <string>

#include "etcd/Client.hpp"
#include "etcd/Response.hpp"
#include "vineyard/client/client.h"
#include "vineyard/client/ds/blob.h"
#include "vineyard/common/util/json.h"

#include "fragment/gart_fragment.h"
#include "fragment/iterator.h"
#include "grin/predefine.h"

template <typename T>
struct GRIN_DATATYPE_ENUM {
  static constexpr GRIN_DATATYPE value = GRIN_DATATYPE::Undefined;
};

template <>
struct GRIN_DATATYPE_ENUM<int32_t> {
  static constexpr GRIN_DATATYPE value = GRIN_DATATYPE::Int32;
};

template <>
struct GRIN_DATATYPE_ENUM<uint32_t> {
  static constexpr GRIN_DATATYPE value = GRIN_DATATYPE::UInt32;
};

template <>
struct GRIN_DATATYPE_ENUM<int64_t> {
  static constexpr GRIN_DATATYPE value = GRIN_DATATYPE::Int64;
};

template <>
struct GRIN_DATATYPE_ENUM<uint64_t> {
  static constexpr GRIN_DATATYPE value = GRIN_DATATYPE::UInt64;
};

template <>
struct GRIN_DATATYPE_ENUM<float> {
  static constexpr GRIN_DATATYPE value = GRIN_DATATYPE::Float;
};

template <>
struct GRIN_DATATYPE_ENUM<double> {
  static constexpr GRIN_DATATYPE value = GRIN_DATATYPE::Double;
};

template <>
struct GRIN_DATATYPE_ENUM<std::string> {
  static constexpr GRIN_DATATYPE value = GRIN_DATATYPE::String;
};

// TODO(wanglei): support more date types
std::string GetDataTypeName(GRIN_DATATYPE);
GRIN_DATATYPE StringToDataType(std::string);

unsigned _grin_get_type_from_property(unsigned long long int);

unsigned _grin_get_prop_from_property(unsigned long long int);

unsigned long long int _grin_create_property(unsigned, unsigned);

#define GRIN_OID_T int64_t
#define GRIN_VID_T uint64_t

typedef gart::GartFragment<GRIN_OID_T, GRIN_VID_T> GRIN_GRAPH_T;
typedef GRIN_GRAPH_T::vertex_t _GRIN_VERTEX_T;

#ifdef GRIN_WITH_VERTEX_ORIGINAL_ID
typedef GRIN_GRAPH_T::oid_t GRIN_VERTEX_ORIGINAL_ID_T;
#endif

#ifdef GRIN_ENABLE_VERTEX_LIST
struct GRIN_VERTEX_LIST_T {
  GRIN_VERTEX_TYPE vtype;
  unsigned all_master_mirror;
};  // 0: all, 1: master, 2 minor
#endif

#ifdef GRIN_ENABLE_VERTEX_LIST_ITERATOR
typedef gart::VertexIterator GRIN_VERTEX_LIST_ITERATOR_T;
#endif

#ifdef GRIN_ENABLE_ADJACENT_LIST_ITERATOR
struct GRIN_ADJACENT_LIST_ITERATOR_T {
  gart::EdgeIterator edge_iter;
  GRIN_DIRECTION dir;
  GRIN_VERTEX v;
  GRIN_EDGE_TYPE etype;
};
#endif

#ifdef GRIN_ENABLE_GRAPH_PARTITION
struct GRIN_PARTITIONED_GRAPH_T {
  std::string etcd_endpoint;
  size_t total_partition_num;
  std::vector<size_t> local_partition_list;
  int read_epoch;
  std::string meta_prefix;
};

typedef std::vector<size_t> GRIN_PARTITION_LIST_T;
#endif

#ifdef GRIN_WITH_VERTEX_PROPERTY
typedef std::vector<GRIN_GRAPH_T::label_id_t> GRIN_VERTEX_TYPE_LIST_T;
typedef std::vector<GRIN_VERTEX_PROPERTY> GRIN_VERTEX_PROPERTY_LIST_T;
#endif

#ifdef GRIN_WITH_EDGE_PROPERTY
typedef GRIN_GRAPH_T::label_id_t GRIN_EDGE_TYPE_T;
typedef std::vector<GRIN_GRAPH_T::label_id_t> GRIN_EDGE_TYPE_LIST_T;
typedef std::vector<GRIN_EDGE_PROPERTY> GRIN_EDGE_PROPERTY_LIST_T;
#endif

#if defined(GRIN_WITH_VERTEX_PROPERTY) || defined(GRIN_WITH_EDGE_PROPERTY)
typedef std::vector<const void*> GRIN_ROW_T;
#endif

class URI {
 public:
  URI(const std::string& uri) {
    std::string::size_type pos = uri.find("://");
    if (pos == std::string::npos) {
      std::cout << "Invalid URI: " << uri;
    }
    protocol = uri.substr(0, pos);
    std::string::size_type pos2 = uri.find("?", pos + 3);
    if (pos2 == std::string::npos) {
      etcd_endpoint = uri.substr(pos + 3);
    } else {
      etcd_endpoint = uri.substr(pos + 3, pos2 - pos - 3);
      std::string params_str = uri.substr(pos2 + 1);
      std::vector<std::string> params_vec;
      std::istringstream f(params_str);
      std::string str;
      while (std::getline(f, str, '&')) {
        auto equal_pos = str.find("=");
        auto key = str.substr(0, equal_pos);
        auto value = str.substr(equal_pos + 1);
        params[key] = value;
      }
    }
  }
  const std::string& getProtocol() const { return protocol; }
  const std::string& getEtcdEndpoint() const { return etcd_endpoint; }
  const std::map<std::string, std::string>& getParams() const { return params; }

 private:
  std::string protocol;
  std::string etcd_endpoint;
  std::map<std::string, std::string> params;
};

#endif  // RESEARCH_GART_GRIN_SRC_PREDEFINE_H_
