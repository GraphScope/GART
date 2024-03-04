from networkx.classes.reportviews import NodeView as _NodeView
from networkx.classes.reportviews import NodeDataView as _NodeDataView

from collections.abc import Mapping, Set


class NodeView(_NodeView):
    __slots__ = (
        "_graph",
        "_nodes",
    )

    def __getstate__(self):
        return {"_graph": self._graph, "_nodes": self._nodes}

    def __setstate__(self, state):
        self._graph = state["_graph"]
        self._nodes = state["_nodes"]

    def __init__(self, graph):
        self._graph = graph
        self._nodes = graph._nodes

    # Mapping methods
    def __len__(self):
        return self._graph.number_of_nodes()

    def __iter__(self):
        return self._graph.__iter__()

    # DataView method
    def __call__(self, data=False, default=None):
        if data is False:
            return self
        return NodeDataView(self, data, default)

    def data(self, data=True, default=None):
        if data is False:
            return self
        return NodeDataView(self, data, default)


class NodeDataView(_NodeDataView):
    def __init__(self, nodeview, data=False, default=None):
        self._data = data
        self._default = default
        self._nodeview = nodeview
        self._index = 0
        self._iter = self._nodeview.__iter__()

    def __len__(self):
        return self._nodeview._graph.number_of_nodes()

    def __iter__(self):
        data = self._data
        if data is False:
            return self._nodeview.__iter__()
        elif data is True:
            return self
        for node in self._nodeview._graph:
            node_data = self._nodeview._graph.get_node_attr(node)
            if data in node_data:
                yield (node, node_data[data])
            else:
                yield (node, self._default)

    def __next__(self):
        node = next(self._iter)
        node_data = self._nodeview._graph.get_node_attr(node)
        return (node, node_data)


class OutEdgeDataView:
    def __init__(self, graph, nbunch=None, data=False, *, default=None):
        self._graph = graph._graph
        self._nbunch = nbunch
        self._data = data
        self._default = default

    def __len__(self):
        if self._nbunch is None:
            return self._graph.number_of_edges()
        num = 0
        for n in self._nbunch:
            num += len(self._graph.get_successors(n))
        return num

    def __iter__(self):
        if self._nbunch is None:
            if self._data is False:
                for src in self._graph:
                    for dst in self._graph.successors(src):
                        yield (src, dst)
            else:
                for src in self._graph:
                    dsts = self._graph.get_successors(src)
                    dsts_data = self._graph.get_succ_attr(src)
                    results = dict(zip(dsts, dsts_data))
                    for dst in results:
                        if self._data is True:
                            yield (src, dst, results[dst])
                        else:
                            yield (
                                src,
                                dst,
                                results[dst].setdefault(self._data, self._default),
                            )

        else:
            if self._data is False:
                for n in self._nbunch:
                    for nbr in self._graph.successors(n):
                        yield (n, nbr)
            else:
                for n in self._nbunch:
                    dsts = self._graph.get_successors(n)
                    dsts_data = self._graph.get_succ_attr(n)
                    results = dict(zip(dsts, dsts_data))
                    for dst in results:
                        if self._data is True:
                            yield (n, dst, results[dst])
                        else:
                            yield (
                                n,
                                dst,
                                results[dst].setdefault(self._data, self._default),
                            )

    def __contains__(self, e):
        u, v = e[:2]
        if self._nbunch is not None and u not in self._nbunch and v not in self._nbunch:
            return False
        return self._graph.has_successor(u, v)
    

class InEdgeDataView(OutEdgeDataView):   
    def __len__(self):
        if self._nbunch is None:
            return self._graph.number_of_edges()
        num = 0
        for n in self._nbunch:
            num += len(self._graph.get_predecessors(n))
        return num

    def __iter__(self):
        if self._nbunch is None:
            if self._data is False:
                for dst in self._graph:
                    for src in self._graph.predecessors(dst):
                        yield (src, dst)
            else:
                for dst in self._graph:
                    srcs = self._graph.get_predecessors(dst)
                    srcs_data = self._graph.get_pred_attr(dst)
                    results = dict(zip(srcs, srcs_data))
                    for src in results:
                        if self._data is True:
                            yield (src, dst, results[src])
                        else:
                            yield (
                                src,
                                dst,
                                results[src].setdefault(self._data, self._default),
                            )

        else:
            if self._data is False:
                for n in self._nbunch:
                    for nbr in self._graph.predecessors(n):
                        yield (nbr, n)
            else:
                for n in self._nbunch:
                    srcs = self._graph.get_predecessors(n)
                    srcs_data = self._graph.get_pred_attr(n)
                    results = dict(zip(srcs, srcs_data))
                    for src in results:
                        if self._data is True:
                            yield (src, n, results[src])
                        else:
                            yield (
                                src,
                                n,
                                results[src].setdefault(self._data, self._default),
                            )


class OutEdgeView(Set, Mapping):
    """A EdgeView class for outward edges of a DiGraph"""

    dataview = OutEdgeDataView

    def __init__(self, graph):
        self._graph = graph

    # Set methods
    def __len__(self):
        return self._graph.number_of_edges()

    def __iter__(self):
        for src in self._graph:
            for dst in self._graph.successors(src):
                yield (src, dst)

    def __contains__(self, e):
        src, dst = e
        return self._graph.has_successor(src, dst)

    def __getitem__(self, e):
        return self._graph.get_edge_data(*e)

    def __call__(self, nbunch=None, data=False, *, default=None):
        if nbunch is None and data is False:
            return self
        return self.dataview(self, nbunch, data, default=default)

    def data(self, data=True, default=None, nbunch=None):
        if nbunch is None and data is False:
            return self
        return self.dataview(self, nbunch, data, default=default)


class EdgeView(OutEdgeView):
    def __len__(self):
        return self._graph.number_of_edges()
    
    
class InEdgeView(EdgeView):
    dataview = InEdgeDataView
     
    def __iter__(self):
        for dst in self._graph:
            for src in self._graph.predecessors(dst):
                yield (src, dst)
    
    
        
    
    
    
    
