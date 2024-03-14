WITH RECURSIVE ShortestPath AS (
  SELECT
    p_id AS vertex,
    0 AS distance,
    ARRAY[p_id] AS path,
    ARRAY[p_id] AS visited,
    1 AS depth -- 添加递归深度计数器
  FROM person
  WHERE p_id = 933
  UNION ALL
  SELECT
    -- 考虑无向边，我们需要选择 `k.src` 和 `k.dst` 中的 "另一端"
    CASE
      WHEN sp.vertex = k.src THEN k.dst
      WHEN sp.vertex = k.dst THEN k.src
    END AS vertex,
    sp.distance + 1 AS distance,
    sp.path || CASE
      WHEN sp.vertex = k.src THEN k.dst
      WHEN sp.vertex = k.dst THEN k.src
    END AS path,
    sp.visited || CASE
      WHEN sp.vertex = k.src THEN k.dst
      WHEN sp.vertex = k.dst THEN k.src
    END AS visited,
    sp.depth + 1 AS depth -- 递增深度
  FROM ShortestPath sp
  JOIN knows k ON (sp.vertex = k.src OR sp.vertex = k.dst) -- 修改这里的 JOIN 条件
  WHERE NOT (CASE
      WHEN sp.vertex = k.src THEN k.dst
      WHEN sp.vertex = k.dst THEN k.src
    END) = ANY(sp.visited)
    AND sp.depth < 5 -- 限制递归深度
)
SELECT vertex, distance, path
FROM ShortestPath
WHERE vertex = 2199023256077
ORDER BY distance
LIMIT 10;
