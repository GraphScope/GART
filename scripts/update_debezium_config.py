#!/usr/bin/env python3

import etcd3
import json
import sys
from urllib.parse import urlparse
import time
import yaml

config_file_path = "/etc/debezium-connector-config/mysql-connector.json"
etcd_endpoint = sys.argv[1]
etcd_prefix = sys.argv[2]
db_name = sys.argv[3]
db_type = sys.argv[4]

with open(config_file_path, "r", encoding="UTF-8") as f:
    debezium_config = json.load(f)

if not etcd_endpoint.startswith(("http://", "https://")):
    etcd_endpoint = "http://" + etcd_endpoint
parsed_url = urlparse(etcd_endpoint)
etcd_host = parsed_url.netloc.split(":")[0]
etcd_port = parsed_url.port
etcd_client = etcd3.client(host=etcd_host, port=etcd_port)

rg_mapping_key = etcd_prefix + "gart_rg_mapping_yaml"
while True:
    try:
        rg_mapping_str, _ = etcd_client.get(rg_mapping_key)
        if rg_mapping_str is not None:
            rg_mapping_str = rg_mapping_str.decode("utf-8")
            break
        time.sleep(5)
    except Exception as e:
        time.sleep(5)

graph_schema = yaml.load(rg_mapping_str, Loader=yaml.SafeLoader)

# Extract the 'vertex_types' list from the dictionary
vertex_types = graph_schema.get("vertexMappings", {}).get("vertex_types", [])
# Iterate through 'vertex_types' and collect 'dataSourceName' values
vertex_table_names = [vertex_type.get("dataSourceName") for vertex_type in vertex_types]

vertex_type_names = [vertex_type.get("type_name") for vertex_type in vertex_types]

# build a dict (vertex_type_name, vertex_table_name)
vertex_type_name_table_mapping = {
    vertex_type_name: vertex_table_name
    for vertex_type_name, vertex_table_name in zip(
        vertex_type_names, vertex_table_names
    )
}

edge_types = graph_schema.get("edgeMappings", {}).get("edge_types", [])
edge_table_names = [edge.get("dataSourceName") for edge in edge_types]

src_names = [edge["type_pair"]["source_vertex"] for edge in edge_types]
dst_names = [edge["type_pair"]["destination_vertex"] for edge in edge_types]

all_table_names = []
both_vertex_edge_table_names = []
for vertex_table_name in vertex_table_names:
    if vertex_table_name not in edge_table_names:
        all_table_names.append(vertex_table_name)
    else:
        both_vertex_edge_table_names.append(vertex_table_name)

# build a dict (table_name, src/dst_type_names)
edge_table_src_dst_type_mapping = {}

for idx, edge_table_name in enumerate(edge_table_names):
    if edge_table_name not in both_vertex_edge_table_names:
        continue

    src_name = src_names[idx]
    dst_name = dst_names[idx]
    if vertex_type_name_table_mapping[src_name] != edge_table_name:
        if edge_table_name not in edge_table_src_dst_type_mapping:
            edge_table_src_dst_type_mapping[edge_table_name] = [src_name]
        else:
            edge_table_src_dst_type_mapping[edge_table_name].append(src_name)
    if vertex_type_name_table_mapping[dst_name] != edge_table_name:
        if edge_table_name not in edge_table_src_dst_type_mapping:
            edge_table_src_dst_type_mapping[edge_table_name] = [dst_name]
        else:
            edge_table_src_dst_type_mapping[edge_table_name].append(dst_name)

both_vertex_edge_table_placed = [0] * len(both_vertex_edge_table_names)
while True:
    if sum(both_vertex_edge_table_placed) == len(both_vertex_edge_table_names):
        break
    for idx, edge_table_name in enumerate(both_vertex_edge_table_names):
        if both_vertex_edge_table_placed[idx] == 1:
            continue
        src_dst_type_names = edge_table_src_dst_type_mapping[edge_table_name]
        if all(
            vertex_type_name_table_mapping[src_dst_type_name] in all_table_names
            for src_dst_type_name in src_dst_type_names
        ):
            all_table_names.append(edge_table_name)
            both_vertex_edge_table_placed[idx] = 1

for edge_table_name in edge_table_names:
    if edge_table_name not in all_table_names:
        all_table_names.append(edge_table_name)

if db_type == "postgresql":
    db_name = "public"
new_line_list = [db_name + "." + table_name for table_name in all_table_names]
new_line = ",".join(new_line_list)
debezium_config["config"]["table.include.list"] = new_line
print(json.dumps(debezium_config))
