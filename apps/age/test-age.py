#!/usr/bin/env python3

import age
from age_dijkstra import Age_Dijkstra
from age_dijkstra import Graph
import time

conn = age.connect(graph="ldbc", dbname="ldbc", user="dbuser", password="123456")

sql = """
MATCH paths = (a:Person {id: "2199023256077"})-[:knows*1..4]-(b:Person {id: "933"})
    WITH paths, relationships(paths) AS rels
    UNWIND rels AS rel
    WITH nodes(paths) AS nodes,
         COLLECT(rel) AS knowRels,
         COUNT(rel) AS totalLength
    LIMIT 10
    RETURN nodes
"""
# cursor = conn.execCypher(sql)
# row = cursor.fetchone()
# cnt = 0
# while row:
#     row = cursor.fetchone()
#     cnt += 1

# print("Total paths: ", cnt)

con = Age_Dijkstra()
con.connect(
    host="localhost", dbname="ldbc", user="dbuser", password="123456", graph_name="ldbc"
)

start_t = time.time()
nodes = []
edges = con.get_all_edge() or []
init_graph = {}
for node in con.get_all_vertices() or []:
    nodes.append(node["id"])

for node in nodes:
    init_graph[node] = {}
for edge in edges:
    v1 = edge["v1"]["id"]
    v2 = edge["v2"]["id"]
    dist = 1
    init_graph[v1][v2] = dist
    # init_graph[v2][v1] = dist  # undirected graph

graph = Graph(nodes, init_graph)

end = time.time()
cons_time = 1000 * (end - start_t)
print("Construct time:", cons_time, "ms")

start_node = "933"

previous_nodes, shortest_path = Graph.dijkstra_algorithm(
    graph=graph, start_node=start_node
)

end = time.time()
print("Shortest time:", 1000 * (end - start_t) - cons_time, "ms")
print("Time:", 1000 * (end - start_t), "ms")

filtered_shortest_path = {
    k: v for k, v in shortest_path.items() if v != 9223372036854775807
}

# print(filtered_shortest_path)

# Graph.print_shortest_path(
#     previous_nodes, shortest_path, start_node=start_node, target_node="21990232555650"
# )
