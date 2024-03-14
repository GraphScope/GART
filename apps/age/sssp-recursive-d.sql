WITH RECURSIVE ShortestPath AS (
  SELECT
    p_id AS vertex,
    0 AS distance,
    ARRAY[p_id] AS path,
    ARRAY[p_id] AS visited
  FROM person
  WHERE p_id = 933 -- 指定起点ID

  UNION ALL

  SELECT
    k.dst AS vertex,
    sp.distance + 1 AS distance,
    sp.path || k.dst AS path,
    sp.visited || k.dst AS visited
  FROM ShortestPath sp
  JOIN knows k ON sp.vertex = k.src
  WHERE NOT k.dst = ANY(sp.visited)
    AND sp.vertex <> 2199023256077 -- 这个条件确保我们只在没有到达目标节点时继续递归
    AND sp.distance < 5 -- 这个条件确保我们只在距离小于5时继续递归
)
SELECT vertex, distance, path
FROM ShortestPath
WHERE vertex = 2199023256077 -- 选择到达目标ID的路径
ORDER BY distance
LIMIT 10; -- 由于边权重为10，第一个结果即为最短路径
