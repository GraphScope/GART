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

from networkx.classes.reportviews import DegreeView
import networkx as nx

import gart.proto.types_pb2 as pb2
import gart.proto.types_pb2_grpc as pb2_grpc


class DiGraph(object):
    def __init__(self, service_port):
        # Increase the maximum message size the client can receive
        channel_options = [
            ('grpc.max_receive_message_length', 1024 * 1024 * 100)  # 100MB
        ]
        channel = grpc.insecure_channel(service_port, options=channel_options)
        self.stub = pb2_grpc.QueryGraphServiceStub(channel)
        self.graph = {}  # store graph schema
        self._nodes = {}
        self.nodes_is_loaded = False
        self._adj = AdjListDict(self)
        self._succ = self._adj
        self._pred = AdjListDict(self, pred=True)

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

    def _get_nodes(self):
        response_iterator = self.stub.getData(pb2.Request(op=pb2.NODES, args=""))
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        result = msgpack.unpackb(arc.get_bytes(), use_list=False)
        # result is a tuple of tuples
        # (label, oid)
        return result

    def __iter__(self):
        """Iterate over the nodes. Use: 'for n in G'."""
        if not self.nodes_is_loaded:
            self._nodes = self._get_nodes()
            self.nodes_is_loaded = True
        return iter(self._nodes)

    def __contains__(self, n):
        """Returns True if n is a node, False otherwise. Use: 'n in G'."""
        try:
            arg = json.dumps(n).encode("utf-8", errors="ignore")
            response_iterator = self.stub.getData(
                pb2.Request(op=pb2.HAS_NODE, args=arg)
            )
            total_response = bytes()
            for response in response_iterator:
                total_response += response.result
            arc = OutArchive(total_response)
            return arc.get_bool()
        except (TypeError, KeyError):
            return False

    def __len__(self):
        """Returns the number of nodes in the graph. Use: 'len(G)'."""
        if not self.nodes_is_loaded:
            response_iterator = self.stub.getData(pb2.Request(op=pb2.NODE_NUM, args=""))
            total_response = bytes()
            for response in response_iterator:
                total_response += response.result
            arc = OutArchive(total_response)
            return arc.get_size()
        return len(self._nodes)

    def __getitem__(self, n):
        """Returns a dict of neighbors of node n.  Use: 'G[n]'."""
        return self.get_succ_neighbor_attr_pair(n)

    # LRU Caches
    @lru_cache(1000)
    def get_node_attr(self, n):
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response_iterator = self.stub.getData(pb2.Request(op=pb2.NODE_DATA, args=arg))
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        return msgpack.unpackb(arc.get_bytes(), use_list=False)

    @lru_cache(1000)
    def get_successors(self, n):
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response_iterator = self.stub.getData(
            pb2.Request(op=pb2.SUCCS_BY_NODE, args=arg)
        )
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        return msgpack.unpackb(arc.get_bytes(), use_list=False)

    @lru_cache(1000)
    def get_succ_attr(self, n):
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response_iterator = self.stub.getData(
            pb2.Request(op=pb2.SUCC_ATTR_BY_NODE, args=arg)
        )
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        return msgpack.unpackb(arc.get_bytes(), use_list=False)

    @lru_cache(1000)
    def get_predecessors(self, n):
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response_iterator = self.stub.getData(
            pb2.Request(op=pb2.PREDS_BY_NODE, args=arg)
        )
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        return msgpack.unpackb(arc.get_bytes(), use_list=False)

    @lru_cache(1000)
    def get_pred_attr(self, n):
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response_iterator = self.stub.getData(
            pb2.Request(op=pb2.PRED_ATTR_BY_NODE, args=arg)
        )
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        return msgpack.unpackb(arc.get_bytes(), use_list=False)

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
        """Returns the number of nodes in the graph."""
        if not self.nodes_is_loaded:
            response_iterator = self.stub.getData(pb2.Request(op=pb2.NODE_NUM, args=""))
            total_response = bytes()
            for response in response_iterator:
                total_response += response.result
            arc = OutArchive(total_response)
            return arc.get_size()
        return len(self._nodes)

    def order(self):
        return self.number_of_nodes()

    @lru_cache(1000)
    def has_node(self, n):
        """Returns True if the graph contains the node n."""
        try:
            arg = json.dumps(n).encode("utf-8", errors="ignore")
            response_iterator = self.stub.getData(
                pb2.Request(op=pb2.HAS_NODE, args=arg)
            )
            total_response = bytes()
            for response in response_iterator:
                total_response += response.result
            arc = OutArchive(total_response)
            return arc.get_bool()
        except (TypeError, KeyError):
            return False

    @lru_cache(1000)
    def has_successor(self, u, v):
        """Returns True if the edge (u, v) is in the graph."""
        edge = (u, v)
        arg = json.dumps(edge).encode("utf-8", errors="ignore")
        response_iterator = self.stub.getData(pb2.Request(op=pb2.HAS_EDGE, args=arg))
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        return arc.get_bool()

    @lru_cache(1000)
    def has_predecessor(self, u, v):
        """Returns True if the edge (v, u) is in the graph."""
        edge = (v, u)
        arg = json.dumps(edge).encode("utf-8", errors="ignore")
        response_iterator = self.stub.getData(pb2.Request(op=pb2.HAS_EDGE, args=arg))
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        return arc.get_bool()

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
        return iter(self.get_successors(n))

    @lru_cache(1000)
    def successors(self, n):
        """Returns an iterator over successor nodes of n."""
        return iter(self.get_successors(n))

    @lru_cache(1000)
    def predecessors(self, n):
        """Returns an iterator over predecessor nodes of n."""
        return iter(self.get_predecessors(n))

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
        edges_num = 0
        if u is None:
            response_iterator = self.stub.getData(pb2.Request(op=pb2.EDGE_NUM, args=""))
            total_response = bytes()
            for response in response_iterator:
                total_response += response.result
            arc = OutArchive(total_response)
            edges_num = arc.get_size()
        elif self.has_edge(u, v):
            edges_num = 1
        return edges_num

    def get_edge_data(self, u, v, default=None):
        """Returns the attribute dictionary associated with edge (u, v)."""
        if not self.has_edge(u, v):
            return default
        edge = (u, v)
        arg = json.dumps(edge).encode("utf-8", errors="ignore")
        response_iterator = self.stub.getData(pb2.Request(op=pb2.EDGE_DATA, args=arg))
        total_response = bytes()
        for response in response_iterator:
            total_response += response.result
        arc = OutArchive(total_response)
        return msgpack.unpackb(arc.get_bytes(), use_list=False)

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
        return DegreeView(self)
    
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
