from collections.abc import Mapping

class AdjListDict(Mapping):
    def __init__(self, graph, pred=False):
        self._graph = graph
        self._pred = pred
        
    def __contains__(self, key):
        return self._graph.has_node(key)
    
    def __len__(self):
        return self._graph.number_of_nodes()
    
    def __getitem__(self, key):
        if self._pred is False:
            return self._graph.get_succ_neighbor_attr_pair(key)
        return self._graph.get_pred_neighbor_attr_pair(key)
    
    def __iter__(self):
        return self._graph.__iter__()
                
class NeighborDict(Mapping):
    def __init__(self, graph):
        self._graph = graph
    
    def __iter__(self):
        for node in self._graph:
            yield (node, self._graph.get_succ_neighbor_attr_pair(node))
            
    def __getitem__(self, key):
        return self._graph.get_succ_neighbor_attr_pair(key)
    
    def __contains__(self, key):
        return self._graph.has_node(key)
    
    def __len__(self):
        return self._graph.number_of_nodes()