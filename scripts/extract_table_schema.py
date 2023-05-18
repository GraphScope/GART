#/usr/bin/env python3

import argparse
import pymysql
import json
import os


def get_parser():
    parser = argparse.ArgumentParser(
        description="Launch database schema extracter for MySQL")
    parser.add_argument("--mysql_host", default="127.0.0.1", help="MySQL host")
    parser.add_argument("--mysql_port", default=3306, help="MySQL port")
    parser.add_argument("--mysql_user", help="MySQL user")
    parser.add_argument("--mysql_password", help="MySQL password")
    parser.add_argument("--mysql_db", help="MySQL database")

    parser.add_argument("--v6d_socket", default="/var/run/vinyard.sock",
                        help="Vinyard socket")
    parser.add_argument("--etcd_endpoint", default="127.0.0.1:2379",
                        help="Etcd endpoint")

    parser.add_argument("--config_file",
                        default="../schema/rgmapping-ldbc.json",
                        help="Config file (RGMapping)")
    parser.add_argument("--output", default="../schema/db_schema.json",
                        help="Output file (database schema)")

    return parser


def exetract_schema(cursor, config_file, output):
    with open(config_file, "r") as f:
        config = json.load(f)
    tables = config["types"]
    schema = {}
    for table in tables:
        table_name = table["table_name"]
        sql = "SHOW COLUMNS FROM " + table_name
        cursor.execute(sql)
        results = cursor.fetchall()
        schema[table_name] = results
    with open(output, "w") as f:
        json.dump(schema, f, indent=4)


if __name__ == "__main__":
    parser = get_parser()
    args = parser.parse_args()
    db = pymysql.connect(
        host=args.mysql_host,
        user=args.mysql_user,
        password=args.mysql_password,
        port=args.mysql_port,
        database=args.mysql_db,
    )
    cursor = db.cursor()
    exetract_schema(cursor, args.config_file, args.output)
