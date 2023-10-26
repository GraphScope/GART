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

#include "util/inline_str.h"

#if defined(GRIN_ENABLE_SCHEMA) && defined(GRIN_WITH_VERTEX_PROPERTY)
GRIN_VERTEX_PROPERTY_LIST grin_get_vertex_property_list_by_type(
    GRIN_GRAPH g, GRIN_VERTEX_TYPE vt) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  auto vpl = new GRIN_VERTEX_PROPERTY_LIST_T();
  auto vertex_prop_num = _g->vertex_property_num(vt);
  for (auto idx = 0; idx < vertex_prop_num; idx++) {
    vpl->push_back(_grin_create_property(vt, idx));
  }
  return vpl;
}

GRIN_VERTEX_TYPE grin_get_vertex_type_from_property(GRIN_GRAPH g,
                                                    GRIN_VERTEX_PROPERTY vp) {
  return _grin_get_type_from_property(vp);
}

const char* grin_get_vertex_property_name(GRIN_GRAPH g, GRIN_VERTEX_TYPE vtype,
                                          GRIN_VERTEX_PROPERTY vp) {
  auto _schema = static_cast<GRIN_GRAPH_T*>(g)->vertex_property2name_map;
  return _schema
      ->find(std::make_pair(_grin_get_type_from_property(vp),
                            _grin_get_prop_from_property(vp)))
      ->second.c_str();
}

GRIN_VERTEX_PROPERTY grin_get_vertex_property_by_name(GRIN_GRAPH g,
                                                      GRIN_VERTEX_TYPE vt,
                                                      const char* name) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  std::string prop_name(name);
  if (vt == GRIN_NULL_VERTEX_TYPE) {
    return GRIN_NULL_VERTEX_PROPERTY;
  }
  auto prop_id = _g->GetVertexPropId(vt, prop_name);
  if (prop_id == -1) {
    return GRIN_NULL_VERTEX_PROPERTY;
  }
  return _grin_create_property(vt, (unsigned) prop_id);
}

GRIN_VERTEX_PROPERTY_LIST grin_get_vertex_properties_by_name(GRIN_GRAPH g,
                                                             const char* name) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  std::string prop_name(name);
  auto vps = new GRIN_VERTEX_PROPERTY_LIST_T();
  for (auto idx = 0; idx < _g->vertex_label_num(); idx++) {
    int prop_id = _g->GetVertexPropId(idx, prop_name);
    if (prop_id != -1) {
      vps->push_back(_grin_create_property((unsigned) idx, (unsigned) prop_id));
    }
  }
  if (vps->size() == 0) {
    delete vps;
    return GRIN_NULL_VERTEX_PROPERTY_LIST;
  }
  return vps;
}

GRIN_VERTEX_PROPERTY grin_get_vertex_property_by_id(
    GRIN_GRAPH g, GRIN_VERTEX_TYPE vt, GRIN_VERTEX_PROPERTY_ID pid) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  if ((int) pid >= _g->vertex_property_num(vt)) {
    return GRIN_NULL_VERTEX_PROPERTY;
  }
  return _grin_create_property(vt, pid);
}

GRIN_VERTEX_PROPERTY_ID grin_get_vertex_property_id(GRIN_GRAPH g,
                                                    GRIN_VERTEX_TYPE vt,
                                                    GRIN_VERTEX_PROPERTY vp) {
  if (_grin_get_type_from_property(vp) != vt) {
    return GRIN_NULL_VERTEX_PROPERTY_ID;
  }
  return _grin_get_prop_from_property(vp);
}
#endif

#if defined(GRIN_ENABLE_SCHEMA) && defined(GRIN_WITH_EDGE_PROPERTY)
GRIN_EDGE_PROPERTY_LIST grin_get_edge_property_list_by_type(GRIN_GRAPH g,
                                                            GRIN_EDGE_TYPE et) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  auto epl = new GRIN_EDGE_PROPERTY_LIST_T();
  auto edge_prop_num = _g->edge_property_num(et);
  for (auto idx = 0; idx < edge_prop_num; idx++) {
    epl->push_back(_grin_create_property(et, (unsigned) idx));
  }
  return epl;
}

const char* grin_get_edge_property_name(GRIN_GRAPH g, GRIN_EDGE_TYPE etype,
                                        GRIN_EDGE_PROPERTY ep) {
  auto _schema = static_cast<GRIN_GRAPH_T*>(g)->edge_property2name_map;
  return _schema
      ->find(std::make_pair(_grin_get_type_from_property(ep),
                            _grin_get_prop_from_property(ep)))
      ->second.c_str();
}

GRIN_EDGE_PROPERTY grin_get_edge_property_by_name(GRIN_GRAPH g,
                                                  GRIN_EDGE_TYPE et,
                                                  const char* name) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  std::string prop_name(name);
  auto prop_id = _g->GetEdgePropId(et, prop_name);
  if (prop_id == -1) {
    return GRIN_NULL_EDGE_PROPERTY;
  }
  return _grin_create_property(et, (unsigned) prop_id);
}

GRIN_EDGE_PROPERTY_LIST grin_get_edge_properties_by_name(GRIN_GRAPH g,
                                                         const char* name) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  std::string prop_name(name);
  auto epl = new GRIN_EDGE_PROPERTY_LIST_T();
  for (auto idx = 0; idx < _g->edge_label_num(); idx++) {
    int prop_id = _g->GetEdgePropId(idx, prop_name);
    if (prop_id != -1) {
      epl->push_back(_grin_create_property((unsigned) idx, (unsigned) prop_id));
    }
  }
  if (epl->size() == 0) {
    delete epl;
    return GRIN_NULL_EDGE_PROPERTY_LIST;
  }
  return epl;
}

GRIN_EDGE_TYPE grin_get_edge_type_from_property(GRIN_GRAPH g,
                                                GRIN_EDGE_PROPERTY ep) {
  return _grin_get_type_from_property(ep);
}

GRIN_EDGE_PROPERTY grin_get_edge_property_by_id(GRIN_GRAPH g, GRIN_EDGE_TYPE et,
                                                GRIN_EDGE_PROPERTY_ID pid) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  if ((int) pid >= _g->edge_property_num(et)) {
    return GRIN_NULL_EDGE_PROPERTY;
  }
  return _grin_create_property(et, pid);
}

GRIN_EDGE_PROPERTY_ID grin_get_edge_property_id(GRIN_GRAPH g, GRIN_EDGE_TYPE et,
                                                GRIN_EDGE_PROPERTY ep) {
  if (_grin_get_type_from_property(ep) != et) {
    return GRIN_NULL_EDGE_PROPERTY_ID;
  }
  return _grin_get_prop_from_property(ep);
}
#endif

#ifdef GRIN_WITH_VERTEX_PROPERTY
bool grin_equal_vertex_property(GRIN_GRAPH g, GRIN_VERTEX_PROPERTY vp1,
                                GRIN_VERTEX_PROPERTY vp2) {
  return (vp1 == vp2);
}

void grin_destroy_vertex_property(GRIN_GRAPH g, GRIN_VERTEX_PROPERTY vp) {}

GRIN_DATATYPE grin_get_vertex_property_datatype(GRIN_GRAPH g,
                                                GRIN_VERTEX_PROPERTY vp) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  std::string dtype_str = _g->GetVertexPropDataType(
      _grin_get_type_from_property(vp), _grin_get_prop_from_property(vp));
  return StringToDataType(dtype_str);
}
#endif

#ifdef GRIN_WITH_EDGE_PROPERTY
bool grin_equal_edge_property(GRIN_GRAPH g, GRIN_EDGE_PROPERTY ep1,
                              GRIN_EDGE_PROPERTY ep2) {
  return (ep1 == ep2);
}

void grin_destroy_edge_property(GRIN_GRAPH g, GRIN_EDGE_PROPERTY ep) {}

GRIN_DATATYPE grin_get_edge_property_datatype(GRIN_GRAPH g,
                                              GRIN_EDGE_PROPERTY ep) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  std::string dtype_str = _g->GetEdgePropDataType(
      _grin_get_type_from_property(ep), _grin_get_prop_from_property(ep));
  return StringToDataType(dtype_str);
}
#endif
