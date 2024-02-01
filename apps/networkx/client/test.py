from digraph import DiGraph

g = DiGraph("localhost:50051")

print("number of nodes:", len(g))

for node in g:
    #print(node, " in g is ", node in g)
    pass
    #print(node)
    
node = (4, 933)
print(g[node])