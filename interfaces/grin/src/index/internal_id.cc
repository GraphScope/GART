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

#include "grin/include/include/index/internal_id.h"

#if defined(GRIN_ENABLE_VERTEX_INTERNAL_ID_INDEX) && \
    !defined(GRIN_ENABLE_SCHEMA)
/**
 * @brief Get the int64 internal id of a vertex
 * @param GRIN_GRAPH The graph
 * @param GRIN_VERTEX The vertex
 * @return The int64 internal id of the vertex
 */
long long int grin_get_vertex_internal_id(GRIN_GRAPH, GRIN_VERTEX);

/**
 * @brief Get the vertex by internal id.
 * Different from pk_of_int64, the internal id is unique over all vertex types.
 * @param GRIN_GRAPH The graph
 * @param id The internal id of the vertex
 * @return The vertex
 */
GRIN_VERTEX grin_get_vertex_by_internal_id(GRIN_GRAPH, long long int id);

/**
 * @brief Get the upper bound of internal id.
 * @param GRIN_GRAPH The graph
 * @return The upper bound
 */
long long int grin_get_vertex_internal_id_upper_bound(GRIN_GRAPH);

/**
 * @brief Get the lower bound of internal id.
 * @param GRIN_GRAPH The graph
 * @return The lower bound
 */
long long int grin_get_vertex_internal_id_lower_bound(GRIN_GRAPH);
#endif

#if defined(GRIN_ENABLE_VERTEX_INTERNAL_ID_INDEX) && defined(GRIN_ENABLE_SCHEMA)
/**
 * @brief Get the int64 internal id of a vertex
 * @param GRIN_GRAPH The graph
 * @param GRIN_VERTEX The vertex
 * @return The int64 internal id of the vertex
 */
long long int grin_get_vertex_internal_id_by_type(GRIN_GRAPH g,
                                                  GRIN_VERTEX_TYPE vt,
                                                  GRIN_VERTEX v) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  _GRIN_VERTEX_T _v(v);
  if (_g->IsInnerVertex(_v)) {
    return _g->GetOffset(_v);
  } else {
    return _g->GetOffset(_v) + _g->GetMaxInnerVerticesNum(vt);
  }
}

/**
 * @brief Get the vertex by internal id under type
 * @param GRIN_GRAPH The graph
 * @param GRIN_VERTEX_TYPE The vertex type
 * @param id The internal id of the vertex under type
 * @return The vertex
 */
GRIN_VERTEX grin_get_vertex_by_internal_id_by_type(GRIN_GRAPH g,
                                                   GRIN_VERTEX_TYPE vt,
                                                   long long int id) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  if (!_g->InternalIdIsValid(id, vt)) {
    return GRIN_NULL_VERTEX;
  }
  if ((uint64_t) id < _g->GetMaxInnerVerticesNum(vt)) {
    return _g->vid_parser.GenerateId(0, vt, id);
  } else {
    auto offset =
        _g->GetMaxOuterIdOffset() - (id - _g->GetMaxInnerVerticesNum(vt));
    return _g->vid_parser.GenerateId(0, vt, offset);
  }
}

/**
 * @brief Get the upper bound of internal id under type.
 * @param GRIN_GRAPH The graph
 * @param GRIN_VERTEX_TYPE The vertex type
 * @return The upper bound of internal id under type
 */
long long int grin_get_vertex_internal_id_upper_bound_by_type(
    GRIN_GRAPH g, GRIN_VERTEX_TYPE vt) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  return _g->GetMaxInnerVerticesNum(vt) + _g->GetMaxOuterVerticesNum(vt);
}

/**
 * @brief Get the lower bound internal id under type.
 * @param GRIN_GRAPH The graph
 * @param GRIN_VERTEX_TYPE The vertex type
 * @return The lower bound internal id under type
 */
long long int grin_get_vertex_internal_id_lower_bound_by_type(
    GRIN_GRAPH g, GRIN_VERTEX_TYPE vt) {
  return 0;
}
#endif