#!/usr/bin/env python3

import argparse
import os
import sys

from urllib.parse import urlparse
import shutil

import etcd3
import yaml


def get_parser():
    parser = argparse.ArgumentParser(
        description="Update Kafka config file",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--kafka_endpoint", help="Kafka endpoint")
    parser.add_argument("--db_type", help="Database type (MySql or Postgresql)")
    parser.add_argument("--db_host", help="Database host")
    parser.add_argument("--db_port", help="Database port")
    parser.add_argument("--db_user", help="Database user")
    parser.add_argument("--db_password", help="Database password")
    parser.add_argument("--db_name", help="Database name")
    parser.add_argument("--enable_bulkload", help="Enable bulkload")
    parser.add_argument("--etcd_endpoint", help="Etcd endpoint")
    parser.add_argument("--etcd_prefix", help="Etcd prefix")
    return parser


def get_etcd_client(etcd_endpoint):
    if not etcd_endpoint.startswith(("http://", "https://")):
        etcd_endpoint = "http://" + etcd_endpoint

    parsed_url = urlparse(etcd_endpoint)
    etcd_host = parsed_url.netloc.split(":")[0]
    etcd_port = parsed_url.port
    return etcd3.client(host=etcd_host, port=etcd_port)


if __name__ == "__main__":
    arg_parser = get_parser()
    args = arg_parser.parse_args()

    kafka_home = os.getenv("KAFKA_HOME")

    if not kafka_home:
        print("KAFKA_HOME is not set")
        sys.exit(1)

    db_type = args.db_type.lower()

    kafka_server = args.kafka_endpoint
    kafka_port = kafka_server.split(":")[1]

    if db_type not in ["mysql", "postgresql"]:
        print("Invalid database type")
        sys.exit(1)

    if db_type == "mysql":
        kafka_config_file_name = (
            kafka_home + "/config/connect-debezium-mysql.properties"
        )
        temp_file_name = kafka_home + "/config/connect-debezium-mysql.properties.tmp"
    else:
        kafka_config_file_name = (
            kafka_home + "/config/connect-debezium-postgresql.properties"
        )
        temp_file_name = (
            kafka_home + "/config/connect-debezium-postgresql.properties.tmp"
        )

    etcd_client = get_etcd_client(args.etcd_endpoint)

    enable_bulkload = (
        args.enable_bulkload == "1"
        or args.enable_bulkload == 1
        or args.enable_bulkload is True
        or args.enable_bulkload.lower() == "true"
    )

    with open(kafka_config_file_name, "r", encoding="UTF-8") as file, open(
        temp_file_name, "w", encoding="UTF-8"
    ) as temp_file:
        for line in file:
            if line.startswith("database.hostname"):
                temp_file.write(f"database.hostname={args.db_host}\n")
            elif line.startswith("database.port"):
                temp_file.write(f"database.port={args.db_port}\n")
            elif line.startswith("database.user"):
                temp_file.write(f"database.user={args.db_user}\n")
            elif line.startswith("database.password"):
                temp_file.write(f"database.password={args.db_password}\n")
            elif line.startswith("database.dbname"):
                temp_file.write(f"database.dbname={args.db_name}\n")
            elif line.startswith("database.include.list"):
                temp_file.write(f"database.include.list={args.db_name}\n")
            elif line.startswith("table.include.list"):
                rg_mapping_key = args.etcd_prefix + "gart_rg_mapping_yaml"

                raw_value = etcd_client.get(rg_mapping_key)
                if raw_value is None or raw_value[0] is None:
                    print(f"ERROR: Key {rg_mapping_key} not found in etcd")
                    exit(1)
                else:
                    rg_mapping_str = raw_value[0].decode("utf-8")

                graph_schema = yaml.load(rg_mapping_str, Loader=yaml.SafeLoader)

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

                for idx, edge_table_name in enumerate(edge_table_names):
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
                    for idx, edge_table_name in enumerate(both_vertex_edge_table_names):
                        if both_vertex_edge_table_placed[idx] == 1:
                            continue
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

                db_name = args.db_name
                if db_type == "postgresql":
                    db_name = "public"
                new_line_list = [
                    db_name + "." + table_name for table_name in all_table_names
                ]
                new_line = ",".join(new_line_list)
                temp_file.write(f"table.include.list={new_line}\n")
            elif line.startswith("snapshot.mode"):
                if enable_bulkload:
                    if db_type == "postgresql":
                        temp_file.write("snapshot.mode=always\n")
                    else:
                        temp_file.write("snapshot.mode=initial\n")
                else:
                    temp_file.write("snapshot.mode=never\n")
            elif line.startswith("database.history.kafka.bootstrap.servers"):
                temp_file.write(
                    f"database.history.kafka.bootstrap.servers=localhost:{kafka_port}\n"
                )
            elif line.startswith("schema.history.internal.kafka.bootstrap.servers"):
                temp_file.write(
                    f"schema.history.internal.kafka.bootstrap.servers=localhost:{kafka_port}\n"
                )
            elif line.startswith("slot.name"):
                # slot.name should be unique for each database
                # this method is a effective way to make it unique
                temp_file.write(f"slot.name=debezium_{args.db_name}\n")
            else:
                temp_file.write(line)
    shutil.move(temp_file_name, kafka_config_file_name)
