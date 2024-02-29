"""
Shortest path algorithms for unweighted graphs.
"""
import warnings

from gart import DiGraph

import networkx as nx

def single_source_shortest_path_length(G, source, cutoff=None):
    """Compute the shortest path lengths from source to all reachable nodes.

    Parameters
    ----------
    G : NetworkX graph

    source : node
       Starting node for path

    cutoff : integer, optional
        Depth to stop the search. Only paths of length <= cutoff are returned.

    Returns
    -------
    lengths : dict
        Dict keyed by node to shortest path length to source.

    Examples
    --------
    >>> G = nx.path_graph(5)
    >>> length = nx.single_source_shortest_path_length(G, 0)
    >>> length[4]
    4
    >>> for node in length:
    ...     print(f"{node}: {length[node]}")
    0: 0
    1: 1
    2: 2
    3: 3
    4: 4

    See Also
    --------
    shortest_path_length
    """
    if source not in G:
        raise nx.NodeNotFound(f"Source {source} is not in G")
    if cutoff is None:
        cutoff = float("inf")
    nextlevel = [source]
    return dict(_single_shortest_path_length(G._adj, nextlevel, cutoff))


def _single_shortest_path_length(adj, firstlevel, cutoff):
    """Yields (node, level) in a breadth first search

    Shortest Path Length helper function
    Parameters
    ----------
        adj : dict
            Adjacency dict or view
        firstlevel : list
            starting nodes, e.g. [source] or [target]
        cutoff : int or float
            level at which we stop the process
    """
    seen = set(firstlevel)
    nextlevel = firstlevel
    level = 0
    n = len(adj)
    for v in nextlevel:
        yield (v, level)
    while nextlevel and cutoff > level:
        level += 1
        thislevel = nextlevel
        nextlevel = []
        for v in thislevel:
            for w in adj[v]:
                if w not in seen:
                    seen.add(w)
                    nextlevel.append(w)
                    yield (w, level)
            if len(seen) == n:
                return

g = DiGraph("localhost:50051")
length = single_source_shortest_path_length(g, (0,0))
for node in length:
    print(f"{node}: {length[node]}")
print("len of length:", len(length))