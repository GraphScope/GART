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

#include "grin/include/include/topology/adjacentlist.h"

#if defined(GRIN_ENABLE_ADJACENT_LIST) && !defined(GRIN_ENABLE_EDGE_PROPERTY)
GRIN_ADJACENT_LIST grin_get_adjacent_list(GRIN_GRAPH, GRIN_DIRECTION,
                                          GRIN_VERTEX);
#endif

#ifdef GRIN_ENABLE_ADJACENT_LIST
void grin_destroy_adjacent_list(GRIN_GRAPH g, GRIN_ADJACENT_LIST adj_list) {}
#endif

#ifdef GRIN_ENABLE_ADJACENT_LIST_ITERATOR
GRIN_ADJACENT_LIST_ITERATOR grin_get_adjacent_list_begin(
    GRIN_GRAPH g, GRIN_ADJACENT_LIST adj_list) {
  auto _g = static_cast<GRIN_GRAPH_T*>(g)->frag;
  auto iter = new GRIN_ADJACENT_LIST_ITERATOR_T();
  iter->v = adj_list.v;
  iter->etype = adj_list.etype;
  if (adj_list.dir == GRIN_DIRECTION::IN) {
    iter->edge_iter =
        _g->GetIncomingAdjList(_GRIN_VERTEX_T(adj_list.v), adj_list.etype);
    iter->dir = GRIN_DIRECTION::IN;
  } else if (adj_list.dir == GRIN_DIRECTION::OUT) {
    iter->edge_iter =
        _g->GetOutgoingAdjList(_GRIN_VERTEX_T(adj_list.v), adj_list.etype);
    iter->dir = GRIN_DIRECTION::OUT;
  }
  return iter;
}

void grin_destroy_adjacent_list_iter(GRIN_GRAPH g,
                                     GRIN_ADJACENT_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_ADJACENT_LIST_ITERATOR_T*>(iter);
  delete _iter;
}

void grin_get_next_adjacent_list_iter(GRIN_GRAPH g,
                                      GRIN_ADJACENT_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_ADJACENT_LIST_ITERATOR_T*>(iter);
  _iter->edge_iter.next();
}

bool grin_is_adjacent_list_end(GRIN_GRAPH g, GRIN_ADJACENT_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_ADJACENT_LIST_ITERATOR_T*>(iter);
  return !_iter->edge_iter.valid();
}

GRIN_VERTEX grin_get_neighbor_from_adjacent_list_iter(
    GRIN_GRAPH g, GRIN_ADJACENT_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_ADJACENT_LIST_ITERATOR_T*>(iter);
  return _iter->edge_iter.neighbor().GetValue();
}

GRIN_EDGE grin_get_edge_from_adjacent_list_iter(
    GRIN_GRAPH g, GRIN_ADJACENT_LIST_ITERATOR iter) {
  auto _iter = static_cast<GRIN_ADJACENT_LIST_ITERATOR_T*>(iter);
  GRIN_EDGE edge;
  if (_iter->dir == GRIN_DIRECTION::IN) {
    edge.src = _iter->edge_iter.neighbor().GetValue();
    edge.dst = _iter->v;
  } else {
    edge.src = _iter->v;
    edge.dst = _iter->edge_iter.neighbor().GetValue();
  }
  edge.dir = _iter->dir;
  edge.etype = _iter->etype;
  edge.edata = _iter->edge_iter.get_data();
  return edge;
}
#endif