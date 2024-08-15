#!/usr/bin/env python3

import etcd3
from urllib.parse import urlparse
import json
import yaml
import os
import time

output_file_path = "/tmp/graph_schema_for_portal.yaml"
json_output_file_path = "/tmp/graph_schema_for_portal.json"

property_data_type_mapping = {}

property_data_type_mapping["int"] = {"primitive_type": "DT_SIGNED_INT32"}
property_data_type_mapping["integer"] = {"primitive_type": "DT_SIGNED_INT32"}
property_data_type_mapping["bigint"] = {"primitive_type": "DT_SIGNED_INT64"}
property_data_type_mapping["float"] = {"primitive_type": "DT_FLOAT"}
property_data_type_mapping["double precision"] = {"primitive_type": "DT_FLOAT"}
property_data_type_mapping["varchar"] = {"string": {"long_text": ""}}
property_data_type_mapping["character varying"] = {"string": {"long_text": ""}}
property_data_type_mapping["text"] = {"string": {"long_text": ""}}
# TODO(wanglei): add more data type mapping like date, timestamp, etc.


etcd_server = os.getenv("ETCD_SERVICE", "etcd")
if not etcd_server.startswith(("http://", "https://")):
    etcd_server = f"http://{etcd_server}"
parsed_url = urlparse(etcd_server)
etcd_host = parsed_url.netloc.split(":")[0]
etcd_port = parsed_url.port
etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
etcd_prefix = os.getenv("ETCD_PREFIX", "gart_meta_")

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

rg_mapping = yaml.load(rg_mapping_str, Loader=yaml.SafeLoader)

table_schema_key = etcd_prefix + "gart_table_schema"
while True:
    try:
        table_schema_str, _ = etcd_client.get(table_schema_key)
        if table_schema_str is not None:
            table_schema_str = table_schema_str.decode("utf-8")
            break
        time.sleep(5)
    except Exception as e:
        time.sleep(5)

table_schema = json.loads(table_schema_str)

result_dict = {}

result_dict["name"] = "graph_schema"
result_dict["description"] = "graph schema for portal"
vertex_types_array = []
edge_types_array = []
schema_dict = {}

vertex_types = rg_mapping["vertexMappings"]["vertex_types"]
vertex_type_number = len(vertex_types)

for idx in range(vertex_type_number):
    vertex_type_dict = {}
    vertex_type_dict["type_name"] = vertex_types[idx]["type_name"]
    table_name = vertex_types[idx]["dataSourceName"]
    property_mappings = vertex_types[idx]["mappings"]
    properties_array = []
    property_info_array = table_schema[table_name]
    for prop_idx in range(len(property_mappings)):
        property_dict = {}
        property_dict["property_name"] = property_mappings[prop_idx]["property"]
        table_cloumn_name = property_mappings[prop_idx]["dataField"]["name"]
        for i in range(len(property_info_array)):
            if property_info_array[i][0] == table_cloumn_name:
                if property_info_array[i][1].startswith("varchar"):
                    property_dict["property_type"] = property_data_type_mapping[
                        "varchar"
                    ]
                else:
                    property_dict["property_type"] = property_data_type_mapping.get(
                        property_info_array[i][1], property_data_type_mapping["varchar"]
                    )
                break
        properties_array.append(property_dict)
    vertex_type_dict["properties"] = properties_array
    vertex_types_array.append(vertex_type_dict)


edge_types = rg_mapping["edgeMappings"]["edge_types"]
edge_type_number = len(edge_types)
for idx in range(edge_type_number):
    edge_type_dict = {}
    edge_type_dict["type_name"] = edge_types[idx]["type_pair"]["edge"]
    vertex_type_pair_relation = {}
    vertex_type_pair_relation["source_vertex"] = edge_types[idx]["type_pair"][
        "source_vertex"
    ]
    vertex_type_pair_relation["destination_vertex"] = edge_types[idx]["type_pair"][
        "destination_vertex"
    ]
    # FIXME: hard code to MANY_TO_MANY
    vertex_type_pair_relation["relation"] = "MANY_TO_MANY"
    edge_type_dict["vertex_type_pair_relations"] = [vertex_type_pair_relation]
    edge_type_dict["directed"] = not edge_types[idx]["type_pair"].get(
        "undirected", False
    )
    properties_array = []
    property_mappings = edge_types[idx]["dataFieldMappings"]
    table_name = edge_types[idx]["dataSourceName"]
    property_info_array = table_schema[table_name]
    for prop_idx in range(len(property_mappings)):
        property_dict = {}
        property_dict["property_name"] = property_mappings[prop_idx]["property"]
        table_cloumn_name = property_mappings[prop_idx]["dataField"]["name"]
        for i in range(len(property_info_array)):
            if property_info_array[i][0] == table_cloumn_name:
                if property_info_array[i][1].startswith("varchar"):
                    property_dict["property_type"] = property_data_type_mapping[
                        "varchar"
                    ]
                else:
                    property_dict["property_type"] = property_data_type_mapping.get(
                        property_info_array[i][1], property_data_type_mapping["varchar"]
                    )
                break
        properties_array.append(property_dict)
    edge_type_dict["properties"] = properties_array
    edge_types_array.append(edge_type_dict)


schema_dict["vertex_types"] = vertex_types_array
schema_dict["edge_types"] = edge_types_array
result_dict["schema"] = schema_dict

with open(json_output_file_path, "w") as f:
    json.dump(result_dict, f, indent=4)


def json_to_yaml(json_file, yaml_file):
    with open(json_file, "r") as jf:
        data = json.load(jf)

    with open(yaml_file, "w") as yf:
        yaml.dump(data, yf, default_flow_style=False)


json_to_yaml(json_output_file_path, output_file_path)
