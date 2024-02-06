from networkx.classes.reportviews import NodeView as _NodeView
from networkx.classes.reportviews import NodeDataView as _NodeDataView

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
        if data is True:
            return self
        return (
            (n, dd[data] if data in dd else self._default)
            for n, dd in self._nodes.items()
        )
        
    def __next__(self):
        node = next(self._iter)
        node_data = self._nodeview._graph.get_node_attr(node)
        print((node, node_data))
        return (node, node_data)