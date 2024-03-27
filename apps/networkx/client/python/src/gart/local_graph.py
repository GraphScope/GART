import sys
import grpc
import msgpack
import json
import os
from functools import cached_property
from functools import lru_cache

from gart.archieve import OutArchive
from gart.reportviews import NodeView
from gart.reportviews import EdgeView
from gart.reportviews import InEdgeView
from gart.dict_factory import AdjListDict
from gart.dict_factory import NeighborDict
from gart.coreviews import AdjacencyView

from networkx.classes.reportviews import DiDegreeView
from networkx.classes.reportviews import InDegreeView
from networkx.classes.reportviews import OutDegreeView

import local_gart


class LocalDiGraph(object):
    def __init__(self, etcd_endpoint, meta_prefix, version):
        # Increase the maximum message size the client can receive
        builder = local_gart.FragmentBuilder(etcd_endpoint, meta_prefix, version)
        self.graph = builder.get_graph()
        self._adj = AdjListDict(self)
        self._succ = self._adj
        self._pred = AdjListDict(self, pred=True)
        self._version = version

    @cached_property
    def adj(self):
        return AdjacencyView(self._adj)

    @cached_property
    def succ(self):
        return AdjacencyView(self._succ)

    @cached_property
    def pred(self):
        return AdjacencyView(self._pred)

    @property
    def name(self):
        """String identifier of the graph.

        This graph attribute appears in the attribute dict G.graph
        keyed by the string `"name"`. as well as an attribute (technically
        a property) `G.name`. This is entirely user controlled.
        """
        return self.graph.get("name", "")

    @name.setter
    def name(self, s):
        self.graph["name"] = s

    def __str__(self):
        """Returns a short summary of the graph."""
        return "A digraph from GART storage."

    def is_multigraph(self):
        return False

    def is_directed(self):
        return True

    def __iter__(self):
        """Iterate over the nodes. Use: 'for n in G'."""
        v_label_num = self.graph.vertex_label_num()
        for v_label in range(v_label_num):
            vertex_iter = self.graph.inner_vertices(v_label)
            v_label_str = self.graph.get_vertex_label_name(v_label)
            while vertex_iter.valid():
                vertex = vertex_iter.vertex()
                vertex_oid = self.graph.get_id(vertex)
                yield (v_label_str, vertex_oid)
                vertex_iter.next()

    def __contains__(self, n):
        """Returns True if n is a node, False otherwise. Use: 'n in G'."""
        v_label = n[0]
        v_oid = n[1]
        v_label_id = self.graph.get_vertex_label_id(v_label)
        return self.graph.oid2gid(v_label_id, v_oid)[0]

    def __len__(self):
        """Returns the number of nodes in the graph. Use: 'len(G)'."""
        return self.graph.get_vertices_num()

    def __getitem__(self, n):
        """Returns a dict of neighbors of node n.  Use: 'G[n]'."""
        v_label_str = n[0]
        v_label = self.graph.get_vertex_label_id(v_label_str)
        v_oid = n[1]
        result = {}
        exist, vertex = self.graph.oid2gid(v_label, v_oid)
        if exist:
            e_label_num = self.graph.edge_label_num()
            for e_label in range(e_label_num):
                e_prop_num = self.graph.edge_property_num(e_label)
                edge_iter = self.graph.get_outgoing_adjList(vertex, e_label)
                while edge_iter.valid():
                    dst_vertex = edge_iter.neighbor()
                    dst_label_id = self.graph.vertex_label(dst_vertex)
                    dst_label_str = self.graph.get_vertex_label_name(dst_label_id)
                    dst_oid = self.graph.get_id(dst_vertex)
                    edge_prop_dict = {}
                    for e_prop in range(e_prop_num):
                        e_prop_name = self.graph.get_edge_prop_name(e_label, e_prop)
                        e_prop_dtype = self.graph.get_edge_prop_data_type(
                            e_label, e_prop
                        )
                        if e_prop_dtype == "INT":
                            e_prop_value = edge_iter.get_int_data(e_prop)
                        elif e_prop_dtype == "FLOAT":
                            e_prop_value = edge_iter.get_float_data(e_prop)
                        elif e_prop_dtype == "STRING":
                            e_prop_value = edge_iter.get_string_data(e_prop)
                        elif e_prop_dtype == "FLOAT":
                            e_prop_value = edge_iter.get_float_data(e_prop)
                        elif e_prop_dtype == "LONG":
                            e_prop_value = edge_iter.get_long_data(e_prop)
                        edge_prop_dict[e_prop_name] = e_prop_value
                    result[(dst_label_str, dst_oid)] = edge_prop_dict
                    edge_iter.next()
        return result

    # LRU Caches
    @lru_cache(1000)
    def get_node_attr(self, n):
        v_label_str = n[0]
        v_label = self.graph.get_vertex_label_id(v_label_str)
        v_oid = n[1]
        exist, vertex = self.graph.oid2gid(v_label, v_oid)
        result = {}
        if exist:
            v_prop_num = self.graph.vertex_property_num(v_label)
            for prop_id in range(v_prop_num):
                prop_name = self.graph.get_vertex_prop_name(v_label, prop_id)
                prop_dtype = self.graph.get_vertex_prop_data_type(v_label, prop_id)
                if prop_dtype == "INT":
                    prop_value = self.graph.get_int_data(vertex, prop_id)
                elif prop_dtype == "FLOAT":
                    prop_value = self.graph.get_float_data(vertex, prop_id)
                elif prop_dtype == "STRING":
                    prop_value = self.graph.get_string_data(vertex, prop_id)
                elif prop_dtype == "DOUBLE":
                    prop_value = self.graph.get_double_data(vertex, prop_id)
                elif prop_dtype == "LONG":
                    prop_value = self.graph.get_long_data(vertex, prop_id)
                result[prop_name] = prop_value
        return result

    @lru_cache(1000)
    def get_successors(self, n):
        v_label_str = n[0]
        v_label = self.graph.get_vertex_label_id(v_label_str)
        v_oid = n[1]
        exist, vertex = self.graph.oid2gid(v_label, v_oid)
        result = ()
        if exist:
            e_label_num = self.graph.edge_label_num()
            for e_label in range(e_label_num):
                edge_iter = self.graph.get_outgoing_adjList(vertex, e_label)
                while edge_iter.valid():
                    dst_vertex = edge_iter.neighbor()
                    dst_label_id = self.graph.vertex_label(dst_vertex)
                    dst_label_str = self.graph.get_vertex_label_name(dst_label_id)
                    dst_oid = self.graph.get_id(dst_vertex)
                    result += ((dst_label_str, dst_oid),)
                    edge_iter.next()
        return result

    @lru_cache(1000)
    def get_succ_attr(self, n):
        v_label_str = n[0]
        v_label = self.graph.get_vertex_label_id(v_label_str)
        v_oid = n[1]
        exist, vertex = self.graph.oid2gid(v_label, v_oid)
        result = ()
        if exist:
            e_label_num = self.graph.edge_label_num()
            for e_label in range(e_label_num):
                edge_iter = self.graph.get_outgoing_adjList(vertex, e_label)
                while edge_iter.valid():
                    edge_prop_dict = {}
                    e_prop_num = self.graph.edge_property_num(e_label)
                    for e_prop_id in range(e_prop_num):
                        e_prop_name = self.graph.get_edge_prop_name(e_label, e_prop_id)
                        e_prop_dtype = self.graph.get_edge_prop_data_type(
                            e_label, e_prop_id
                        )
                        if e_prop_dtype == "INT":
                            e_prop_value = edge_iter.get_int_data(e_prop_id)
                        elif e_prop_dtype == "FLOAT":
                            e_prop_value = edge_iter.get_float_data(e_prop_id)
                        elif e_prop_dtype == "STRING":
                            e_prop_value = edge_iter.get_string_data(e_prop_id)
                        elif e_prop_dtype == "DOUBLE":
                            e_prop_value = edge_iter.get_double_data(e_prop_id)
                        elif e_prop_dtype == "LONG":
                            e_prop_value = edge_iter.get_long_data(e_prop_id)
                        edge_prop_dict[e_prop_name] = e_prop_value
                    result += (edge_prop_dict,)
                    edge_iter.next()
        return result

    @lru_cache(1000)
    def get_predecessors(self, n):
        v_label_str = n[0]
        v_label = self.graph.get_vertex_label_id(v_label_str)
        v_oid = n[1]
        exist, vertex = self.graph.oid2gid(v_label, v_oid)
        result = ()
        if exist:
            e_label_num = self.graph.edge_label_num()
            for e_label in range(e_label_num):
                edge_iter = self.graph.get_incoming_adjList(vertex, e_label)
                while edge_iter.valid():
                    dst_vertex = edge_iter.neighbor()
                    dst_label_id = self.graph.vertex_label(dst_vertex)
                    dst_label_str = self.graph.get_vertex_label_name(dst_label_id)
                    dst_oid = self.graph.get_id(dst_vertex)
                    result += ((dst_label_str, dst_oid),)
                    edge_iter.next()
        return result

    @lru_cache(1000)
    def get_pred_attr(self, n):
        v_label_str = n[0]
        v_label = self.graph.get_vertex_label_id(v_label_str)
        v_oid = n[1]
        exist, vertex = self.graph.oid2gid(v_label, v_oid)
        result = ()
        if exist:
            e_label_num = self.graph.edge_label_num()
            for e_label in range(e_label_num):
                edge_iter = self.graph.get_incoming_adjList(vertex, e_label)
                while edge_iter.valid():
                    edge_prop_dict = {}
                    e_prop_num = self.graph.edge_property_num(e_label)
                    for e_prop_id in range(e_prop_num):
                        e_prop_name = self.graph.get_edge_prop_name(e_label, e_prop_id)
                        e_prop_dtype = self.graph.get_edge_prop_data_type(
                            e_label, e_prop_id
                        )
                        if e_prop_dtype == "INT":
                            e_prop_value = edge_iter.get_int_data(e_prop_id)
                        elif e_prop_dtype == "FLOAT":
                            e_prop_value = edge_iter.get_float_data(e_prop_id)
                        elif e_prop_dtype == "STRING":
                            e_prop_value = edge_iter.get_string_data(e_prop_id)
                        elif e_prop_dtype == "DOUBLE":
                            e_prop_value = edge_iter.get_double_data(e_prop_id)
                        elif e_prop_dtype == "LONG":
                            e_prop_value = edge_iter.get_long_data(e_prop_id)
                        edge_prop_dict[e_prop_name] = e_prop_value
                    result += (edge_prop_dict,)
                    edge_iter.next()
        return result

    @lru_cache(1000)
    def get_pred_neighbor_attr_pair(self, n):
        neighbors = self.get_predecessors(n)
        neighbors_attr = self.get_pred_attr(n)
        return dict(zip(neighbors, neighbors_attr))

    @lru_cache(1000)
    def get_succ_neighbor_attr_pair(self, n):
        neighbors = self.get_successors(n)
        neighbors_attr = self.get_succ_attr(n)
        return dict(zip(neighbors, neighbors_attr))

    @property
    def nodes(self):
        return NodeView(self)

    def number_of_nodes(self):
        return self.__len__()

    def order(self):
        return self.number_of_nodes()

    @lru_cache(1000)
    def has_node(self, n):
        """Returns True if the graph contains the node n."""
        return self.__contains__(n)

    @lru_cache(1000)
    def has_successor(self, u, v):
        """Returns True if the edge (u, v) is in the graph."""
        src_label_str = u[0]
        src_label = self.graph.get_vertex_label_id(src_label_str)
        src_oid = u[1]
        exist, src_vertex = self.graph.oid2gid(src_label, src_oid)
        if not exist:
            return False
        dst_label_str = v[0]
        dst_label = self.graph.get_vertex_label_id(dst_label_str)
        dst_oid = v[1]
        e_label_num = self.graph.edge_label_num()
        for e_label_id in range(e_label_num):
            edge_iter = self.graph.get_outgoing_adjList(src_vertex, e_label_id)
            while edge_iter.valid():
                dst_vertex = edge_iter.neighbor()
                if self.graph.vertex_label(dst_vertex) != dst_label:
                    break
                if dst_oid == self.graph.get_id(dst_vertex):
                    return True
                edge_iter.next()
        return False

    @lru_cache(1000)
    def has_predecessor(self, u, v):
        """Returns True if the edge (v, u) is in the graph."""
        return self.has_successor(v, u)

    def has_edge(self, u, v):
        """Returns True if the edge (u, v) is in the graph."""
        return self.has_successor(u, v)

    @lru_cache(1000)
    def neighbors(self, n):
        """Returns an iterator over all neighbors of node n.
        Notes
        -----
        neighbors() and successors() are the same.
        """
        v_label_str = n[0]
        v_label = self.graph.get_vertex_label_id(v_label_str)
        v_oid = n[1]
        exist, vertex = self.graph.oid2gid(v_label, v_oid)
        if exist:
            e_label_num = self.graph.edge_label_num()
            for e_label in range(e_label_num):
                edge_iter = self.graph.get_outgoing_adjList(vertex, e_label)
                while edge_iter.valid():
                    dst_vertex = edge_iter.neighbor()
                    dst_label_id = self.graph.vertex_label(dst_vertex)
                    dst_label_str = self.graph.get_vertex_label_name(dst_label_id)
                    dst_oid = self.graph.get_id(dst_vertex)
                    yield (dst_label_str, dst_oid)
                    edge_iter.next()
        else:
            return iter([])

    @lru_cache(1000)
    def successors(self, n):
        """Returns an iterator over successor nodes of n."""
        return self.neighbors(n)

    @lru_cache(1000)
    def predecessors(self, n):
        """Returns an iterator over predecessor nodes of n."""
        v_label_str = n[0]
        v_label = self.graph.get_vertex_label_id(v_label_str)
        v_oid = n[1]
        exist, vertex = self.graph.oid2gid(v_label, v_oid)
        if exist:
            e_label_num = self.graph.edge_label_num()
            for e_label in range(e_label_num):
                edge_iter = self.graph.get_incoming_adjList(vertex, e_label)
                while edge_iter.valid():
                    dst_vertex = edge_iter.neighbor()
                    dst_label_id = self.graph.vertex_label(dst_vertex)
                    dst_label_str = self.graph.get_vertex_label_name(dst_label_id)
                    dst_oid = self.graph.get_id(dst_vertex)
                    yield (dst_label_str, dst_oid)
                    edge_iter.next()
        else:
            return iter([])

    @property
    def edges(self):
        return EdgeView(self)

    @property
    def out_edges(self):
        return EdgeView(self)

    @property
    def in_edges(self):
        return InEdgeView(self)

    @lru_cache(1000)
    def number_of_edges(self, u=None, v=None):
        if u is None:
            return self.graph.get_edge_num()
        elif self.has_successor(u, v):
            return 1
        return 0

    def get_edge_data(self, u, v, default=None):
        """Returns the attribute dictionary associated with edge (u, v)."""
        try:
            return self._adj[u][v]
        except KeyError:
            return default

    def adjacency(self):
        """Returns an iterator over (node, adjacency dict) tuples for all nodes."""
        return iter(NeighborDict(self))

    def size(self, weight=None):
        if weight is None:
            return self.number_of_edges() // 2
        return sum(d for v, d in self.degree(weight=weight)) / 2

    @property
    def degree(self):
        """A DegreeView for the Graph as G.degree or G.degree()."""
        return DiDegreeView(self)

    @property
    def in_degree(self):
        return InDegreeView(self)

    @property
    def out_degree(self):
        return OutDegreeView(self)

    def nbunch_iter(self, nbunch=None):
        if nbunch is None:
            return self.__iter__()
        else:
            try:
                iter(nbunch)
            except TypeError:
                nbunch = [nbunch]
            for n in nbunch:
                if n in self:
                    yield n
