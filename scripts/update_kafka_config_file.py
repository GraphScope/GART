#!/usr/bin/env python3

import argparse
import os
import etcd3
from urllib.parse import urlparse
import yaml
import shutil
import sys


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


if __name__ == "__main__":
    arg_parser = get_parser()
    args = arg_parser.parse_args()

    kafka_home = os.getenv("KAFKA_HOME")

    if not kafka_home:
        print("KAFKA_HOME is not set")
        sys.exit(1)

    db_type = args.db_type

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

    etcd_endpoint = args.etcd_endpoint

    with open(kafka_config_file_name, "r") as file, open(
        temp_file_name, "w"
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
                if not etcd_endpoint.startswith(("http://", "https://")):
                    etcd_endpoint = "http://" + etcd_endpoint
                parsed_url = urlparse(etcd_endpoint)
                etcd_host = parsed_url.netloc.split(":")[0]
                etcd_port = parsed_url.port
                etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
                rg_mapping_key = args.etcd_prefix + "gart_rg_mapping_yaml"
                rg_mapping_str = etcd_client.get(rg_mapping_key)[0].decode("utf-8")
                graph_schema = yaml.load(rg_mapping_str, Loader=yaml.SafeLoader)

                # Extract the 'vertex_types' list from the dictionary
                vertex_types = graph_schema.get("vertexMappings", {}).get(
                    "vertex_types", []
                )
                # Iterate through 'vertex_types' and collect 'dataSourceName' values
                vertex_table_names = [
                    vertex_type.get("dataSourceName") for vertex_type in vertex_types
                ]
                edge_types = graph_schema.get("edgeMappings", {}).get("edge_types", [])
                edge_table_names = [edge.get("dataSourceName") for edge in edge_types]
                all_table_names = vertex_table_names + edge_table_names
                db_name = args.db_name
                if db_type == "postgresql":
                    db_name = "public"
                new_line_list = [
                    db_name + "." + table_name for table_name in all_table_names
                ]
                new_line = ",".join(new_line_list)
                temp_file.write(f"table.include.list={new_line}\n")
            elif line.startswith("snapshot.mode"):
                if (
                    args.enable_bulkload == "1"
                    or args.enable_bulkload == 1
                    or args.enable_bulkload == True
                    or args.enable_bulkload == "True"
                    or args.enable_bulkload == "true"
                ):
                    if db_type == "postgresql":
                        temp_file.write("snapshot.mode=always\n")
                    else:
                        temp_file.write("snapshot.mode=initial\n")
                else:
                    temp_file.write("snapshot.mode=never\n")
            elif line.startswith("database.history.kafka.bootstrap.servers"):
                temp_file.write(
                    f"database.history.kafka.bootstrap.servers={args.kafka_endpoint}\n"
                )
            elif line.startswith("schema.history.internal.kafka.bootstrap.servers"):
                temp_file.write(
                    f"schema.history.internal.kafka.bootstrap.servers={args.kafka_endpoint}\n"
                )
            else:
                temp_file.write(line)
    shutil.move(temp_file_name, kafka_config_file_name)
