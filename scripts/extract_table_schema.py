#!/usr/bin/env python3

import argparse
import json
import sys
import pymysql


def get_parser():
    parser = argparse.ArgumentParser(
        description="Launch database schema extracter for MySQL",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--host", default="127.0.0.1", help="MySQL host")
    parser.add_argument("--port", default=3306, help="MySQL port")
    parser.add_argument("--user", help="MySQL user")
    parser.add_argument("--password", help="MySQL password")
    parser.add_argument("--db", default="my_maxwell_01", help="MySQL database")

    parser.add_argument(
        "--rgmapping_file",
        default="../schema/rgmapping-ldbc.json",
        help="Config file (RGMapping)",
    )
    parser.add_argument(
        "--output",
        default="../schema/db_schema.json",
        help="Output file (database schema)",
    )

    return parser


def exetract_schema(cursor, rgmapping_file, output):
    with open(rgmapping_file, "r", encoding="UTF-8") as f:
        config = json.load(f)
    tables = config["types"]
    schema = {}
    for table in tables:
        table_name = table["table_name"]
        sql = "SHOW COLUMNS FROM " + table_name
        cursor.execute(sql)
        results = cursor.fetchall()
        schema[table_name] = results
    with open(output, "w", encoding="UTF-8") as f:
        json.dump(schema, f, indent=4)


if __name__ == "__main__":
    arg_parser = get_parser()
    args = arg_parser.parse_args()

    unset = False
    if not isinstance(args.user, str) or len(args.user) == 0:
        print("Please specify the MySQL user with --user")
        unset = True

    if not isinstance(args.password, str) or len(args.password) == 0:
        print("Please specify the MySQL password with --password")
        unset = True

    if unset:
        sys.exit(1)

    print("Args: ", args)

    db = pymysql.connect(
        host=args.host,
        port=int(args.port),
        user=args.user,
        password=args.password,
        database=args.db,
    )
    db_cursor = db.cursor()
    exetract_schema(db_cursor, args.rgmapping_file, args.output)
    print(f"Generate schema file: {args.output}")
