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

#include "grin/include/include/topology/vertexlist.h"

#if defined(GRIN_ENABLE_VERTEX_LIST) && !defined(GRIN_WITH_VERTEX_PROPERTY)
GRIN_VERTEX_LIST grin_get_vertex_list(GRIN_GRAPH g) {}
#endif

#ifdef GRIN_ENABLE_VERTEX_LIST
void grin_destroy_vertex_list(GRIN_GRAPH g, GRIN_VERTEX_LIST vl) {
  auto _vl = static_cast<GRIN_VERTEX_LIST_T*>(vl);
  delete _vl;
}
#endif

#ifdef GRIN_ENABLE_VERTEX_LIST_ITERATOR
GRIN_VERTEX_LIST_ITERATOR grin_get_vertex_list_begin(GRIN_GRAPH g,
                                                     GRIN_VERTEX_LIST vl) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g);
  auto _vl = static_cast<GRIN_VERTEX_LIST_T*>(vl);
  auto iter = new GRIN_VERTEX_LIST_ITERATOR_T();
  if (_vl->all_master_mirror == 0) {
    *iter = _g->Vertices(_vl->vtype);
  } else if (_vl->all_master_mirror == 1) {
    *iter = _g->InnerVertices(_vl->vtype);
  } else if (_vl->all_master_mirror == 2) {
    *iter = _g->OuterVertices(_vl->vtype);
  }
  return iter;
}

void grin_destroy_vertex_list_iter(GRIN_GRAPH g,
                                   GRIN_VERTEX_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(iter);
  delete _iter;
}

void grin_get_next_vertex_list_iter(GRIN_GRAPH g,
                                    GRIN_VERTEX_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(iter);
  _iter->next();
}

bool grin_is_vertex_list_end(GRIN_GRAPH g, GRIN_VERTEX_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(iter);
  return !_iter->valid();
}

GRIN_VERTEX grin_get_vertex_from_iter(GRIN_GRAPH g,
                                      GRIN_VERTEX_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_VERTEX_LIST_ITERATOR_T*>(iter);
  return _iter->vertex().GetValue();
}
#endif