import sys
import grpc
from archieve import OutArchive
import msgpack
import json
from functools import cached_property
from functools import lru_cache

import networkx as nx

sys.path.insert(1, "../proto")

import types_pb2 as pb2
import types_pb2_grpc as pb2_grpc


class DiGraph(object):
    def __init__(self, service_port):
        channel = grpc.insecure_channel(service_port)
        self.stub = pb2_grpc.QueryGraphServiceStub(channel)
        self.graph = {} # store graph schema
        self.nodes = {}
        self.nodes_is_loaded = False
        
    @cached_property
    def adj(self):
        pass
    
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
    
    def _get_nodes(self):
        response = self.stub.getData(pb2.Request(op=pb2.NODES, args=""))
        arc = OutArchive(response.result)
        result = msgpack.unpackb(arc.get_bytes(), use_list=False)
        # result is a tuple of tuples
        # (label, oid)
        return result
        
    def __iter__(self):
        """Iterate over the nodes. Use: 'for n in G'."""
        if not self.nodes_is_loaded:
            self.nodes = self._get_nodes()
            self.nodes_is_loaded = True
        return iter(self.nodes)
    
    def __contains__(self, n):
        """Returns True if n is a node, False otherwise. Use: 'n in G'."""
        try:
            if not self.nodes_is_loaded:
                arg = json.dumps(n).encode("utf-8", errors="ignore")
                response = self.stub.getData(pb2.Request(op=pb2.HAS_NODE, args=arg))
                arc = OutArchive(response.result)
                return arc.get_bool()
            else:
                return n in self.nodes
        except (TypeError, KeyError):
            return False

    def __len__(self):
        """Returns the number of nodes in the graph. Use: 'len(G)'."""
        if not self.nodes_is_loaded:
            response = self.stub.getData(pb2.Request(op=pb2.NODE_NUM, args=""))
            arc = OutArchive(response.result)
            return arc.get_size()
        return len(self.nodes)
    
    def __getitem__(self, n):
        """Returns a dict of neighbors of node n.  Use: 'G[n]'."""
        return self.get_succ_neighbor_attr_pair(n)
        
    
    # LRU Caches
    @lru_cache(1000)
    def get_node_attr(self, n):
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response = self.stub.getData(pb2.Request(op=pb2.NODE_DATA, args=arg))
        arc = OutArchive(response.result)
        return msgpack.unpackb(arc.get_bytes(), use_list=False)

    @lru_cache(1000)
    def get_successors(self, n):
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response = self.stub.getData(pb2.Request(op=pb2.SUCCS_BY_NODE, args=arg))
        arc = OutArchive(response.result)
        return msgpack.unpackb(arc.get_bytes(), use_list=False)

    @lru_cache(1000)
    def get_succ_attr(self, n):
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response = self.stub.getData(pb2.Request(op=pb2.SUCC_ATTR_BY_NODE, args=arg))
        arc = OutArchive(response.result)
        return msgpack.unpackb(arc.get_bytes(), use_list=False)

    @lru_cache(1000)
    def get_predecessors(self, n):
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response = self.stub.getData(pb2.Request(op=pb2.PREDS_BY_NODE, args=arg))
        arc = OutArchive(response.result)
        return msgpack.unpackb(arc.get_bytes(), use_list=False)

    @lru_cache(1000)
    def get_pred_attr(self, n):
        arg = json.dumps(n).encode("utf-8", errors="ignore")
        response = self.stub.getData(pb2.Request(op=pb2.PRED_ATTR_BY_NODE, args=arg))
        arc = OutArchive(response.result)
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
    

    