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

extern "C" {
#include "grin/include/property/type.h"
}
#include "grin/src/predefine.h"

#ifdef GRIN_WITH_VERTEX_PROPERTY
// Vertex type
bool grin_equal_vertex_type(GRIN_GRAPH g, GRIN_VERTEX_TYPE vt1,
                            GRIN_VERTEX_TYPE vt2) {
  return (vt1 == vt2);
}

GRIN_VERTEX_TYPE grin_get_vertex_type(GRIN_GRAPH g, GRIN_VERTEX v) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  return _g->vertex_label(_GRIN_VERTEX_T(v));
}

void grin_destroy_vertex_type(GRIN_GRAPH g, GRIN_VERTEX_TYPE vt) {}

// Vertex type list
GRIN_VERTEX_TYPE_LIST grin_get_vertex_type_list(GRIN_GRAPH g) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto vtl = new GRIN_VERTEX_TYPE_LIST_T();
  auto vertex_label_num = _g->vertex_label_num();
  for (auto idx = 0; idx < vertex_label_num; ++idx) {
    vtl->push_back(idx);
  }
  return vtl;
}

void grin_destroy_vertex_type_list(GRIN_GRAPH g, GRIN_VERTEX_TYPE_LIST vtl) {
  auto _vtl = static_cast<GRIN_VERTEX_TYPE_LIST_T*>(vtl);
  delete _vtl;
}

GRIN_VERTEX_TYPE_LIST grin_create_vertex_type_list(GRIN_GRAPH g) {
  auto vtl = new GRIN_VERTEX_TYPE_LIST_T();
  return vtl;
}

bool grin_insert_vertex_type_to_list(GRIN_GRAPH g, GRIN_VERTEX_TYPE_LIST vtl,
                                     GRIN_VERTEX_TYPE vt) {
  auto _vtl = static_cast<GRIN_VERTEX_TYPE_LIST_T*>(vtl);
  _vtl->push_back(vt);
  return true;
}

size_t grin_get_vertex_type_list_size(GRIN_GRAPH g, GRIN_VERTEX_TYPE_LIST vtl) {
  auto _vtl = static_cast<GRIN_VERTEX_TYPE_LIST_T*>(vtl);
  return _vtl->size();
}

GRIN_VERTEX_TYPE grin_get_vertex_type_from_list(GRIN_GRAPH g,
                                                GRIN_VERTEX_TYPE_LIST vtl,
                                                size_t idx) {
  auto _vtl = static_cast<GRIN_VERTEX_TYPE_LIST_T*>(vtl);
  return (*_vtl)[idx];
}
#endif

#ifdef GRIN_WITH_VERTEX_TYPE_NAME
const char* grin_get_vertex_type_name(GRIN_GRAPH g, GRIN_VERTEX_TYPE vt) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  std::string type_name = _g->GetVertexLabelName(vt);
  auto len = type_name.length() + 1;
  char* out = new char[len];
  snprintf(out, len, "%s", type_name.c_str());
  return out;
}

GRIN_VERTEX_TYPE grin_get_vertex_type_by_name(GRIN_GRAPH g, const char* name) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  std::string type_name(name);
  auto type = _g->GetVertexLabelId(type_name);
  if (type == -1) {
    return GRIN_NULL_VERTEX_TYPE;
  }
  return type;
}
#endif

#ifdef GRIN_TRAIT_NATURAL_ID_FOR_VERTEX_TYPE
GRIN_VERTEX_TYPE_ID grin_get_vertex_type_id(GRIN_GRAPH g, GRIN_VERTEX_TYPE vt) {
  return vt;
}

GRIN_VERTEX_TYPE grin_get_vertex_type_by_id(GRIN_GRAPH g,
                                            GRIN_VERTEX_TYPE_ID tid) {
  return tid;
}
#endif

#ifdef GRIN_WITH_EDGE_PROPERTY
// Edge type
bool grin_equal_edge_type(GRIN_GRAPH g, GRIN_EDGE_TYPE et1,
                          GRIN_EDGE_TYPE et2) {
  return (et1 == et2);
}

GRIN_EDGE_TYPE grin_get_edge_type(GRIN_GRAPH g, GRIN_EDGE e) {
  auto _e = static_cast<GRIN_EDGE_T*>(e);
  return _e->etype;
}

void grin_destroy_edge_type(GRIN_GRAPH g, GRIN_EDGE_TYPE et) {
  // do nothing
}

// Edge type list
GRIN_EDGE_TYPE_LIST grin_get_edge_type_list(GRIN_GRAPH g) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto etl = new GRIN_EDGE_TYPE_LIST_T();
  auto edge_label_num = _g->edge_label_num();
  for (auto idx = 0; idx < edge_label_num; ++idx) {
    etl->push_back(idx);
  }
  return etl;
}

void grin_destroy_edge_type_list(GRIN_GRAPH g, GRIN_EDGE_TYPE_LIST etl) {
  auto _etl = static_cast<GRIN_EDGE_TYPE_LIST_T*>(etl);
  delete _etl;
}

GRIN_EDGE_TYPE_LIST grin_create_edge_type_list(GRIN_GRAPH g) {
  auto etl = new GRIN_EDGE_TYPE_LIST_T();
  return etl;
}

bool grin_insert_edge_type_to_list(GRIN_GRAPH g, GRIN_EDGE_TYPE_LIST etl,
                                   GRIN_EDGE_TYPE et) {
  auto _etl = static_cast<GRIN_EDGE_TYPE_LIST_T*>(etl);
  _etl->push_back(et);
  return true;
}

size_t grin_get_edge_type_list_size(GRIN_GRAPH g, GRIN_EDGE_TYPE_LIST etl) {
  auto _etl = static_cast<GRIN_EDGE_TYPE_LIST_T*>(etl);
  return _etl->size();
}

GRIN_EDGE_TYPE grin_get_edge_type_from_list(GRIN_GRAPH g,
                                            GRIN_EDGE_TYPE_LIST etl,
                                            size_t idx) {
  auto _etl = static_cast<GRIN_EDGE_TYPE_LIST_T*>(etl);
  return (*_etl)[idx];
}
#endif

#ifdef GRIN_WITH_EDGE_TYPE_NAME
const char* grin_get_edge_type_name(GRIN_GRAPH g, GRIN_EDGE_TYPE et) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  std::string type_name = _g->GetEdgeLabelName(et);
  auto len = type_name.length() + 1;
  char* out = new char[len];
  snprintf(out, len, "%s", type_name.c_str());
  return out;
}

GRIN_EDGE_TYPE grin_get_edge_type_by_name(GRIN_GRAPH g, const char* name) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  std::string type_name(name);
  auto type = _g->GetEdgeLabelId(type_name);
  if (type == -1) {
    return GRIN_NULL_VERTEX_TYPE;
  }
  return type;
}
#endif

#ifdef GRIN_TRAIT_NATURAL_ID_FOR_EDGE_TYPE
GRIN_EDGE_TYPE_ID grin_get_edge_type_id(GRIN_GRAPH g, GRIN_EDGE_TYPE et) {
  return et;
}

GRIN_EDGE_TYPE grin_get_edge_type_by_id(GRIN_GRAPH g, GRIN_EDGE_TYPE_ID etid) {
  return etid;
}
#endif

#if defined(GRIN_WITH_VERTEX_PROPERTY) && defined(GRIN_WITH_EDGE_PROPERTY)
/** @brief  the src vertex type list */
GRIN_VERTEX_TYPE_LIST grin_get_src_types_by_edge_type(GRIN_GRAPH g,
                                                      GRIN_EDGE_TYPE et) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto vtl = new GRIN_VERTEX_TYPE_LIST_T();
  vtl->push_back(
      _g->edge2vertex_map.find((et) + _g->vertex_label_num())->second.first);
  return vtl;
}

/** @brief get the dst vertex type list */
GRIN_VERTEX_TYPE_LIST grin_get_dst_types_by_edge_type(GRIN_GRAPH g,
                                                      GRIN_EDGE_TYPE et) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto vtl = new GRIN_VERTEX_TYPE_LIST_T();
  vtl->push_back(
      _g->edge2vertex_map.find((et) + _g->vertex_label_num())->second.second);
  return vtl;
}

/** @brief get the edge type list related to a given pair of vertex types */
GRIN_EDGE_TYPE_LIST grin_get_edge_types_by_vertex_type_pair(
    GRIN_GRAPH g, GRIN_VERTEX_TYPE vt1, GRIN_VERTEX_TYPE vt2) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto etl = new GRIN_EDGE_TYPE_LIST_T();
  auto etype = _g->vertex2edge_map.find(std::make_pair(vt1, vt2))->second -
               _g->vertex_label_num();
  etl->push_back(etype);
  return etl;
}
#endif
///@}
