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

#include "grin/src/predefine.h"

#include "grin/include/include/property/property.h"
#include "grin/include/include/property/propertylist.h"
#include "grin/include/include/property/row.h"
#include "grin/include/include/property/value.h"

#if defined(GRIN_ENABLE_ROW) && \
    defined(GRIN_TRAIT_PROPERTY_VALUE_OF_FLOAT_ARRAY)
void grin_destroy_row_value_of_float_array(GRIN_GRAPH g, const float* addr,
                                           size_t size) {}

const float* grin_get_float_array_from_row(GRIN_GRAPH g, GRIN_ROW r,
                                           size_t index, size_t* length) {
  // TODO(wanglei): implement this
  assert(false);
  return nullptr;
}

bool grin_insert_float_array_to_row(GRIN_GRAPH g, GRIN_ROW r, const float* addr,
                                    size_t idx) {
  assert(false);
  return false;
}
#endif

#ifdef GRIN_ENABLE_ROW
void grin_destroy_row(GRIN_GRAPH g, GRIN_ROW r) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  delete _r;
}

void grin_destroy_row_value_of_string(GRIN_GRAPH g, const char* addr) {
  delete addr;
}

int grin_get_int32_from_row(GRIN_GRAPH g, GRIN_ROW r, size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  return *static_cast<const int32_t*>((*_r)[idx]);
}

unsigned int grin_get_uint32_from_row(GRIN_GRAPH g, GRIN_ROW r, size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  return *static_cast<const uint32_t*>((*_r)[idx]);
}

long long int grin_get_int64_from_row(GRIN_GRAPH g, GRIN_ROW r, size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  return *static_cast<const int64_t*>((*_r)[idx]);
}

unsigned long long int grin_get_uint64_from_row(GRIN_GRAPH g, GRIN_ROW r,
                                                size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  return *static_cast<const uint64_t*>((*_r)[idx]);
}

float grin_get_float_from_row(GRIN_GRAPH g, GRIN_ROW r, size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  return *static_cast<const float*>((*_r)[idx]);
}

double grin_get_double_from_row(GRIN_GRAPH g, GRIN_ROW r, size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  return *static_cast<const double*>((*_r)[idx]);
}

const char* grin_get_string_from_row(GRIN_GRAPH g, GRIN_ROW r, size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  return static_cast<const char*>((*_r)[idx]);
}

int grin_get_date32_from_row(GRIN_GRAPH g, GRIN_ROW r, size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  return *static_cast<const int32_t*>((*_r)[idx]);
}

int grin_get_time32_from_row(GRIN_GRAPH g, GRIN_ROW r, size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  return *static_cast<const int32_t*>((*_r)[idx]);
}

long long int grin_get_timestamp64_from_row(GRIN_GRAPH g, GRIN_ROW r,
                                            size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  return *static_cast<const int64_t*>((*_r)[idx]);
}

GRIN_ROW grin_create_row(GRIN_GRAPH g) {
  auto r = new GRIN_ROW_T();
  return r;
}

bool grin_insert_int32_to_row(GRIN_GRAPH g, GRIN_ROW r, int value) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  _r->push_back(new int32_t(value));
  return true;
}

bool grin_insert_uint32_to_row(GRIN_GRAPH g, GRIN_ROW r, unsigned int value) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  _r->push_back(new uint32_t(value));
  return true;
}

bool grin_insert_int64_to_row(GRIN_GRAPH g, GRIN_ROW r, long long int value) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  _r->push_back(new int64_t(value));
  return true;
}

bool grin_insert_uint64_to_row(GRIN_GRAPH g, GRIN_ROW r,
                               unsigned long long int value) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  _r->push_back(new uint64_t(value));
  return true;
}

bool grin_insert_float_to_row(GRIN_GRAPH g, GRIN_ROW r, float value) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  _r->push_back(new float(value));
  return true;
}

bool grin_insert_double_to_row(GRIN_GRAPH g, GRIN_ROW r, double value) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  _r->push_back(new double(value));
  return true;
}

bool grin_insert_string_to_row(GRIN_GRAPH g, GRIN_ROW r, const char* value) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  _r->push_back(new std::string(value));
  return true;
}

bool grin_insert_date32_to_row(GRIN_GRAPH g, GRIN_ROW r, int value) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  _r->push_back(new int32_t(value));
  return true;
}

bool grin_insert_time32_to_row(GRIN_GRAPH g, GRIN_ROW r, int value) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  _r->push_back(new int32_t(value));
  return true;
}

bool grin_insert_timestamp64_to_row(GRIN_GRAPH g, GRIN_ROW r,
                                    long long int value) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  _r->push_back(new int64_t(value));
  return true;
}
#endif

#if defined(GRIN_ENABLE_ROW) && defined(GRIN_TRAIT_CONST_VALUE_PTR)
const void* grin_get_value_from_row(GRIN_GRAPH g, GRIN_ROW r, GRIN_DATATYPE dt,
                                    size_t idx) {
  auto _r = static_cast<GRIN_ROW_T*>(r);
  switch (dt) {
  case GRIN_DATATYPE::Int32:
    return static_cast<const int32_t*>((*_r)[idx]);
  case GRIN_DATATYPE::UInt32:
    return static_cast<const uint32_t*>((*_r)[idx]);
  case GRIN_DATATYPE::Int64:
    return static_cast<const int64_t*>((*_r)[idx]);
  case GRIN_DATATYPE::UInt64:
    return static_cast<const uint64_t*>((*_r)[idx]);
  case GRIN_DATATYPE::Float:
    return static_cast<const float*>((*_r)[idx]);
  case GRIN_DATATYPE::Double:
    return static_cast<const double*>((*_r)[idx]);
  case GRIN_DATATYPE::String:
    return static_cast<const char*>((*_r)[idx]);
  case GRIN_DATATYPE::Date32:
    return static_cast<const int32_t*>((*_r)[idx]);
  case GRIN_DATATYPE::Time32:
    return static_cast<const int32_t*>((*_r)[idx]);
  case GRIN_DATATYPE::Timestamp64:
    return static_cast<const int64_t*>((*_r)[idx]);
  default:
    return NULL;
  }
  return NULL;
}
#endif

#if defined(GRIN_WITH_VERTEX_PROPERTY) && defined(GRIN_ENABLE_ROW)
GRIN_ROW grin_get_vertex_row(GRIN_GRAPH g, GRIN_VERTEX v) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  _GRIN_VERTEX_T _v(v);
  auto v_type = _g->vertex_label(_v);
  auto prop_size = _g->vertex_property_num(v_type);
  auto r = new GRIN_ROW_T();
  for (auto prop_id = 0; prop_id < prop_size; prop_id++) {
    std::string dtype_str = _g->GetVertexPropDataType(v_type, prop_id);
    if (dtype_str == "INT") {
      r->push_back(_g->template GetDataAddr<int32_t>(_v, prop_id));
    } else if (dtype_str == "LONG") {
      r->push_back(_g->template GetDataAddr<uint64_t>(_v, prop_id));
    } else if (dtype_str == "FLOAT") {
      r->push_back(_g->template GetDataAddr<float>(_v, prop_id));
    } else if (dtype_str == "DOUBLE") {
      r->push_back(_g->template GetDataAddr<double>(_v, prop_id));
    } else if (dtype_str == "STRING") {
      r->push_back(_g->template GetDataAddr<std::string_view>(_v, prop_id));
    } else {
      r->push_back(NULL);
    }
  }
  return r;
}
#endif

#if defined(GRIN_WITH_EDGE_PROPERTY) && defined(GRIN_ENABLE_ROW)
GRIN_ROW grin_get_edge_row(GRIN_GRAPH g, GRIN_EDGE e) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  auto e_type = e.etype;
  auto prop_size = _g->edge_property_num(e_type);
  auto r = new GRIN_ROW_T();
  for (auto idx = 0; idx < prop_size; idx++) {
    auto ep = grin_get_edge_property_by_id(g, e_type, idx);
    r->push_back(grin_get_edge_property_value(g, e, ep));
  }
  return r;
}
#endif