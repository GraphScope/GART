from gart import DiGraph

g = DiGraph("localhost:50051")

print("number of nodes:", len(g))

# for node in g:
# print(node, " in g is ", node in g)
#    pass
# print(node)

node = (4, 933)
print(g[node])

# for node in g.nodes(data="org_id", default="default"):
#    pass
#    print(node)
# print(g.nodes(data=True))

node = (4, 933)
print(g[node])

print(g._adj[node])

print(len(g[node]) == len(g._adj[node]))

node = (0, 1)
print(g.adj[node])
dst = (4, 24189255812047)
print(g._adj[node][dst])

# for node in g.nodes(data=True):
#    pass
# print(node)
# print(g.nodes(data=True))

# for edge in g.edges(data="wa_work_from", default="default"):
#    print(edge)

# for src in g.adjacency():
#    print(src)
for src in g.degree(weight="span"):
    print(src)
