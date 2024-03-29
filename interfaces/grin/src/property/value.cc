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

#include "grin/include/include/common/error.h"
#include "grin/include/include/property/property.h"
#include "grin/include/include/property/type.h"
#include "grin/include/include/property/value.h"

#if defined(GRIN_WITH_VERTEX_PROPERTY) && \
    defined(GRIN_TRAIT_PROPERTY_VALUE_OF_FLOAT_ARRAY)
void grin_destroy_vertex_property_value_of_float_array(GRIN_GRAPH g,
                                                       const float* addr,
                                                       size_t length) {}

const float* grin_get_vertex_property_value_of_float_array(
    GRIN_GRAPH g, GRIN_VERTEX v, GRIN_VERTEX_PROPERTY vp, size_t* length) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  *length =
      (_g->GetColumnFamilyDataLength(_GRIN_VERTEX_T(v), 0)) / sizeof(float);
  return reinterpret_cast<const float*>(
      _g->GetRowDataAddr(_GRIN_VERTEX_T(v), 0));
}
#endif

#ifdef GRIN_WITH_VERTEX_PROPERTY
void grin_destroy_vertex_property_value_of_string(GRIN_GRAPH g,
                                                  const char* addr) {}

int grin_get_vertex_property_value_of_int32(GRIN_GRAPH g, GRIN_VERTEX v,
                                            GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->template GetData<int32_t>(_GRIN_VERTEX_T(v),
                                       _grin_get_prop_from_property(vp));
}

unsigned int grin_get_vertex_property_value_of_uint32(GRIN_GRAPH g,
                                                      GRIN_VERTEX v,
                                                      GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->template GetData<uint32_t>(_GRIN_VERTEX_T(v),
                                        _grin_get_prop_from_property(vp));
}

long long int grin_get_vertex_property_value_of_int64(GRIN_GRAPH g,
                                                      GRIN_VERTEX v,
                                                      GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->template GetData<int64_t>(_GRIN_VERTEX_T(v),
                                       _grin_get_prop_from_property(vp));
}

unsigned long long int grin_get_vertex_property_value_of_uint64(
    GRIN_GRAPH g, GRIN_VERTEX v, GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->template GetData<uint64_t>(_GRIN_VERTEX_T(v),
                                        _grin_get_prop_from_property(vp));
}

float grin_get_vertex_property_value_of_float(GRIN_GRAPH g, GRIN_VERTEX v,
                                              GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->template GetData<float>(_GRIN_VERTEX_T(v),
                                     _grin_get_prop_from_property(vp));
}

double grin_get_vertex_property_value_of_double(GRIN_GRAPH g, GRIN_VERTEX v,
                                                GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->template GetData<double>(_GRIN_VERTEX_T(v),
                                      _grin_get_prop_from_property(vp));
}

const char* grin_get_vertex_property_value_of_string(GRIN_GRAPH g,
                                                     GRIN_VERTEX v,
                                                     GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->template GetDataAddr<std::string_view>(
      _GRIN_VERTEX_T(v), _grin_get_prop_from_property(vp));
}

int grin_get_vertex_property_value_of_date32(GRIN_GRAPH g, GRIN_VERTEX v,
                                             GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->template GetData<int>(_GRIN_VERTEX_T(v),
                                   _grin_get_prop_from_property(vp));
}

int grin_get_vertex_property_value_of_time32(GRIN_GRAPH g, GRIN_VERTEX v,
                                             GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->template GetData<int>(_GRIN_VERTEX_T(v),
                                   _grin_get_prop_from_property(vp));
}

long long int grin_get_vertex_property_value_of_timestamp64(
    GRIN_GRAPH g, GRIN_VERTEX v, GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->template GetData<uint64_t>(_GRIN_VERTEX_T(v),
                                        _grin_get_prop_from_property(vp));
}

#endif

#if defined(GRIN_WITH_VERTEX_PROPERTY) && defined(GRIN_TRAIT_CONST_VALUE_PTR)
const void* grin_get_vertex_property_value(GRIN_GRAPH g, GRIN_VERTEX v,
                                           GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  auto prop_id = _grin_get_prop_from_property(vp);
  auto v_type = _grin_get_type_from_property(vp);
  std::string dtype_str = _g->GetVertexPropDataType(v_type, prop_id);
  void* _value = NULL;
  if (dtype_str == "INT") {
    _value = const_cast<char*>(
        _g->template GetDataAddr<int32_t>(_GRIN_VERTEX_T(v), prop_id));
  } else if (dtype_str == "LONG") {
    _value = const_cast<char*>(
        _g->template GetDataAddr<uint64_t>(_GRIN_VERTEX_T(v), prop_id));
  } else if (dtype_str == "FLOAT") {
    _value = const_cast<char*>(
        _g->template GetDataAddr<float>(_GRIN_VERTEX_T(v), prop_id));
  } else if (dtype_str == "DOUBLE") {
    _value = const_cast<char*>(
        _g->template GetDataAddr<double>(_GRIN_VERTEX_T(v), prop_id));
  } else if (dtype_str == "STRING") {
    _value = const_cast<char*>(
        _g->template GetDataAddr<std::string_view>(_GRIN_VERTEX_T(v), prop_id));
  } else {
    grin_error_code = GRIN_ERROR_CODE::UNKNOWN_DATATYPE;
    _value = NULL;
  }
  return _value;
}
#endif

#if defined(GRIN_WITH_EDGE_PROPERTY) && \
    defined(GRIN_TRAIT_PROPERTY_VALUE_OF_FLOAT_ARRAY)
void grin_destroy_edge_property_value_of_float_array(GRIN_GRAPH g,
                                                     const float* addr,
                                                     size_t idx) {}

const float* grin_get_edge_property_value_of_float_array(GRIN_GRAPH g,
                                                         GRIN_EDGE e,
                                                         GRIN_EDGE_PROPERTY ep,
                                                         size_t* length) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  auto edge_type = grin_get_edge_type(g, e);
  *length = _g->edge_property_num(edge_type);
  char* e_data = e.edata;
  return reinterpret_cast<float*>(e_data);
}
#endif

#ifdef GRIN_WITH_EDGE_PROPERTY
void grin_destroy_edge_property_value_of_string(GRIN_GRAPH g,
                                                const char* addr) {}

int grin_get_edge_property_value_of_int32(GRIN_GRAPH g, GRIN_EDGE e,
                                          GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  if (prop_id == 0) {
    return *reinterpret_cast<int*>(base_addr);
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    return *reinterpret_cast<int*>(base_addr + offset);
  }
}

unsigned int grin_get_edge_property_value_of_uint32(GRIN_GRAPH g, GRIN_EDGE e,
                                                    GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  if (prop_id == 0) {
    return *reinterpret_cast<uint32_t*>(base_addr);
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    return *reinterpret_cast<uint32_t*>(base_addr + offset);
  }
}

long long int grin_get_edge_property_value_of_int64(GRIN_GRAPH g, GRIN_EDGE e,
                                                    GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  if (prop_id == 0) {
    return *reinterpret_cast<int64_t*>(base_addr);
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    return *reinterpret_cast<int64_t*>(base_addr + offset);
  }
}

unsigned long long int grin_get_edge_property_value_of_uint64(
    GRIN_GRAPH g, GRIN_EDGE e, GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  if (prop_id == 0) {
    return *reinterpret_cast<uint64_t*>(base_addr);
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    return *reinterpret_cast<uint64_t*>(base_addr + offset);
  }
}

float grin_get_edge_property_value_of_float(GRIN_GRAPH g, GRIN_EDGE e,
                                            GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  if (prop_id == 0) {
    return *reinterpret_cast<float*>(base_addr);
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    return *reinterpret_cast<float*>(base_addr + offset);
  }
}

double grin_get_edge_property_value_of_double(GRIN_GRAPH g, GRIN_EDGE e,
                                              GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  if (prop_id == 0) {
    return *reinterpret_cast<double*>(base_addr);
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    return *reinterpret_cast<double*>(base_addr + offset);
  }
}

const char* grin_get_edge_property_value_of_string(GRIN_GRAPH g, GRIN_EDGE e,
                                                   GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  int64_t fake_edata = 0;
  if (prop_id == 0) {
    fake_edata = *reinterpret_cast<int64_t*>(base_addr);
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    fake_edata = *reinterpret_cast<int64_t*>(base_addr + offset);
  }
  int64_t edata_offset = fake_edata >> 16;
  return _g->GetStringBuffer() + edata_offset;
}

int grin_get_edge_property_value_of_date32(GRIN_GRAPH g, GRIN_EDGE e,
                                           GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  if (prop_id == 0) {
    return *reinterpret_cast<int*>(base_addr);
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    return *reinterpret_cast<int*>(base_addr + offset);
  }
}

int grin_get_edge_property_value_of_time32(GRIN_GRAPH g, GRIN_EDGE e,
                                           GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  if (prop_id == 0) {
    return *reinterpret_cast<int*>(base_addr);
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    return *reinterpret_cast<int*>(base_addr + offset);
  }
}

long long int grin_get_edge_property_value_of_timestamp64(
    GRIN_GRAPH g, GRIN_EDGE e, GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  if (prop_id == 0) {
    return *reinterpret_cast<int64_t*>(base_addr);
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    return *reinterpret_cast<int64_t*>(base_addr + offset);
  }
}

#endif

#if defined(GRIN_WITH_EDGE_PROPERTY) && defined(GRIN_TRAIT_CONST_VALUE_PTR)
const void* grin_get_edge_property_value(GRIN_GRAPH g, GRIN_EDGE e,
                                         GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  char* base_addr = e.edata;
  auto prop_id = _grin_get_prop_from_property(ep);
  auto e_type_id = _grin_get_type_from_property(ep);
  auto dt = grin_get_edge_property_datatype(g, ep);
  if (prop_id == 0) {
    if (dt != GRIN_DATATYPE::String) {
      return base_addr;
    } else {
      int64_t fake_edata = *reinterpret_cast<int64_t*>(base_addr);
      int64_t edata_offset = fake_edata >> 16;
      return _g->GetStringBuffer() + edata_offset;
    }
  } else {
    auto offset = _g->edge_prop_offsets[e_type_id][prop_id - 1];
    if (dt != GRIN_DATATYPE::String) {
      return base_addr + offset;
    } else {
      int64_t fake_edata = *reinterpret_cast<int64_t*>(base_addr + offset);
      int64_t edata_offset = fake_edata >> 16;
      return _g->GetStringBuffer() + edata_offset;
    }
  }
}
#endif