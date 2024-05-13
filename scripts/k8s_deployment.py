#!/usr/bin/env python3

import argparse
import json
import sys
import yaml
import os
import etcd3


def get_parser():
    parser = argparse.ArgumentParser(
        description="Launch GART in Kubernetes cluster",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--user_config_path", help="User config file path")
    parser.add_argument("--rg_mapping_file_path", help="RGMapping file path")
    return parser


if __name__ == "__main__":
    arg_parser = get_parser()
    args = arg_parser.parse_args()

    gart_home = os.getenv("GART_HOME")
    if not gart_home:
        print("GART_HOME is not set")
        sys.exit(1)

    with open(args.user_config_path, "r", encoding="UTF-8") as f:
        config = json.load(f)

    etcd_host = "localhost"
    etcd_port = 12379
    # FIXME: read port from yaml file
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)

    with open(args.rg_mapping_file_path, "r", encoding="UTF-8") as f:
        rg_mapping = yaml.safe_load(f)

    etcd_prefix = config.get("etcd_prefix", "gart_meta_")

    etcd_client.put(
        etcd_prefix + "gart_rg_mapping_yaml", yaml.dump(rg_mapping, sort_keys=False)
    )

    db_host = config.get("db_host", "127.0.0.1")
    db_type = config.get("db_type", "mysql")
    db_port = config.get("db_port", 3306 if db_type == "mysql" else 5432)
    db_name = config.get("db_name", "ldbc")
    db_user = config.get("db_user", "root")
    db_password = config.get("db_password", "")
    v6d_socket = config.get("v6d_socket", "/tmp/v6d.sock")
    v6d_size = config.get("v6d_size", "10G")
    total_subgraph_num = config.get("total_subgraph_num", 1)
    enable_bulkload = config.get("enable_bulkload", 1)

    # update gart-config.yaml
    with open(gart_home + "/k8s/gart-config-template.yaml", "r") as file, open(
        gart_home + "/k8s/gart-config.yaml", "w"
    ) as temp_file:
        for line in file:
            if "DB_HOST" in line:
                temp_file.write(line.split(":")[0] + ': "' + db_host + '"\n')
            elif "DB_PORT" in line:
                temp_file.write(line.split(":")[0] + ': "' + str(db_port) + '"\n')
            elif "DB_USER" in line:
                temp_file.write(line.split(":")[0] + ': "' + db_user + '"\n')
            elif "DB_PASSWORD" in line:
                temp_file.write(line.split(":")[0] + ': "' + db_password + '"\n')
            elif "DB_TYPE" in line:
                temp_file.write(line.split(":")[0] + ': "' + db_type + '"\n')
            elif "DB_NAME" in line:
                temp_file.write(line.split(":")[0] + ': "' + db_name + '"\n')
            elif "V6D_SOCKET" in line:
                temp_file.write(line.split(":")[0] + ': "' + v6d_socket + '"\n')
            elif "V6D_SIZE" in line:
                temp_file.write(line.split(":")[0] + ': "' + v6d_size + '"\n')
            elif "TOTAL_SUBGRAPH_NUM" in line:
                temp_file.write(
                    line.split(":")[0] + ': "' + str(total_subgraph_num) + '"\n'
                )
            elif "ENABLE_BULKLOAD" in line:
                temp_file.write(
                    line.split(":")[0] + ': "' + str(enable_bulkload) + '"\n'
                )
            else:
                temp_file.write(line)

    # update writer-deployment.yaml
    with open(gart_home + "/k8s/writer-deployment-template.yaml", "r") as file, open(
        gart_home + "/k8s/writer-deployment.yaml", "w"
    ) as temp_file:
        for line in file:
            if "replicas" in line:
                temp_file.write(
                    line.split(":")[0] + ": " + str(total_subgraph_num) + "\n"
                )
            else:
                temp_file.write(line)

    # update debezium-config.yaml
    with open(gart_home + "/k8s/debezium-config-template.yaml", "r") as file, open(
        gart_home + "/k8s/debezium-config.yaml", "w"
    ) as temp_file:
        for line in file:
            if "database.hostname" in line:
                temp_file.write(line.split(":")[0] + ': "' + db_host + '",\n')
            elif "database.port" in line:
                temp_file.write(line.split(":")[0] + ': "' + str(db_port) + '",\n')
            elif "database.user" in line:
                temp_file.write(line.split(":")[0] + ': "' + db_user + '",\n')
            elif "database.password" in line:
                temp_file.write(line.split(":")[0] + ': "' + db_password + '",\n')
            elif "database.include.list" in line:
                temp_file.write(line.split(":")[0] + ': "' + db_name + '",\n')
            elif "snapshot.mode" in line:
                if enable_bulkload:
                    if db_type == "mysql":
                        temp_file.write(line.split(":")[0] + ': "' + "initial" + '",\n')
                    else:
                        temp_file.write(line.split(":")[0] + ': "' + "always" + '",\n')
                else:
                    temp_file.write(line.split(":")[0] + ': "' + "never" + '",\n')
            elif "table.include.list" in line:
                with open(args.rg_mapping_file_path, "r") as file:
                    graph_schema = yaml.safe_load(file)

                # Extract the 'vertex_types' list from the dictionary
                vertex_types = graph_schema.get("vertexMappings", {}).get(
                    "vertex_types", []
                )
                # Iterate through 'vertex_types' and collect 'dataSourceName' values
                vertex_table_names = [
                    vertex_type.get("dataSourceName") for vertex_type in vertex_types
                ]

                vertex_type_names = [
                    vertex_type.get("type_name") for vertex_type in vertex_types
                ]

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
                dst_names = [
                    edge["type_pair"]["destination_vertex"] for edge in edge_types
                ]

                all_table_names = []
                both_vertex_edge_table_names = []
                for vertex_table_name in vertex_table_names:
                    if vertex_table_name not in edge_table_names:
                        all_table_names.append(vertex_table_name)
                    else:
                        both_vertex_edge_table_names.append(vertex_table_name)

                # build a dict (table_name, src/dst_type_names)
                edge_table_src_dst_type_mapping = {}

                for idx in range(len(edge_table_names)):
                    edge_table_name = edge_table_names[idx]
                    if edge_table_name not in both_vertex_edge_table_names:
                        continue

                    src_name = src_names[idx]
                    dst_name = dst_names[idx]
                    if vertex_type_name_table_mapping[src_name] != edge_table_name:
                        if edge_table_name not in edge_table_src_dst_type_mapping:
                            edge_table_src_dst_type_mapping[edge_table_name] = [
                                src_name
                            ]
                        else:
                            edge_table_src_dst_type_mapping[edge_table_name].append(
                                src_name
                            )
                    if vertex_type_name_table_mapping[dst_name] != edge_table_name:
                        if edge_table_name not in edge_table_src_dst_type_mapping:
                            edge_table_src_dst_type_mapping[edge_table_name] = [
                                dst_name
                            ]
                        else:
                            edge_table_src_dst_type_mapping[edge_table_name].append(
                                dst_name
                            )

                both_vertex_edge_table_placed = [0] * len(both_vertex_edge_table_names)
                while True:
                    if sum(both_vertex_edge_table_placed) == len(
                        both_vertex_edge_table_names
                    ):
                        break
                    for idx in range(len(both_vertex_edge_table_names)):
                        if both_vertex_edge_table_placed[idx] == 1:
                            continue
                        edge_table_name = both_vertex_edge_table_names[idx]
                        src_dst_type_names = edge_table_src_dst_type_mapping[
                            edge_table_name
                        ]
                        if all(
                            vertex_type_name_table_mapping[src_dst_type_name]
                            in all_table_names
                            for src_dst_type_name in src_dst_type_names
                        ):
                            all_table_names.append(edge_table_name)
                            both_vertex_edge_table_placed[idx] = 1

                for edge_table_name in edge_table_names:
                    if edge_table_name not in all_table_names:
                        all_table_names.append(edge_table_name)

                tmp_db_name = db_name
                if tmp_db_name == "postgresql":
                    tmp_db_name = "public"
                new_line_list = [
                    tmp_db_name + "." + table_name for table_name in all_table_names
                ]
                new_line = ",".join(new_line_list)
                temp_file.write(line.split(":")[0] + ': "' + new_line + '",\n')
            else:
                temp_file.write(line)
