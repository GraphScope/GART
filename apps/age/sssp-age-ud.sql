-- Time: 929599.329 ms (15:29.599)
SELECT * FROM cypher('ldbc', $$
    MATCH paths = (a:Person {id: "2199023256077"})-[:knows*1..4]-(b:Person {id: "933"})
    WITH paths, relationships(paths) AS rels
    UNWIND rels AS rel
    WITH nodes(paths) AS nodes,
         COLLECT(rel) AS knowRels,
         COUNT(rel) AS totalLength
    RETURN nodes, knowRels, totalLength
$$) AS (nodes agtype, knowRels agtype, totalLength bigint)
ORDER BY totalLength
LIMIT 10;
