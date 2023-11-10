#!/usr/bin/env python3

import sys

import argparse
import json
from sqlalchemy import create_engine
import yaml


def get_parser():
    parser = argparse.ArgumentParser(
        description="Launch database schema extracter",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--host", default="127.0.0.1", help="Database server host")
    parser.add_argument("--port", default=3306, help="Database server port")
    parser.add_argument("--user", help="Database user")
    parser.add_argument("--password", help="Database password")
    parser.add_argument("--db", default="ldbc", help="Database name")
    parser.add_argument(
        "--db_type", default="mysql", help="Database type: mysql, postgresql"
    )

    parser.add_argument(
        "--rgmapping_file",
        default="schema/rgmapping-ldbc.yaml",
        help="Config file (RGMapping)",
    )
    parser.add_argument(
        "--output",
        default="schema/db_schema.json",
        help="Output file (database schema)",
    )

    parser.add_argument(
        "--output_yaml",
        default="schema/graph.yaml",
        help="Output YAML file (graph schema)",
    )

    return parser


def exetract_schema(cursor, rgmapping_file, database, db_type, output):
    with open(rgmapping_file, "r", encoding="UTF-8") as f:
        config = yaml.safe_load(f)
    tables = (
        config["vertexMappings"]["vertex_types"] + config["edgeMappings"]["edge_types"]
    )
    sum_row = 0
    schema = {}
    for table in tables:
        table_name = table["dataSourceName"]
        sql = ""
        # sql = "SHOW COLUMNS FROM " + table_name
        if db_type == "mysql":
            sql = f"""SELECT COLUMN_NAME, COLUMN_TYPE
                    FROM information_schema.COLUMNS
                    WHERE TABLE_NAME='{table_name}' and TABLE_SCHEMA='{database}'"""
        elif db_type == "postgresql":
            sql = f"""SELECT COLUMN_NAME, DATA_TYPE
            FROM information_schema.COLUMNS
            WHERE TABLE_NAME='{table_name}' and TABLE_CATALOG='{database}'"""
        cursor.execute(sql)
        results = cursor.fetchall()
        schema[table_name] = results

        sql = f"""SELECT COUNT(*) FROM {table_name}"""
        cursor.execute(sql)
        results = cursor.fetchall()
        sum_row += results[0][0]
    with open(output, "w", encoding="UTF-8") as f:
        json.dump(schema, f, indent=4)

    return schema, sum_row


# schema: {table_name: [(column_name, column_type), ...]}
def produce_graph_schema(schema, rgmapping_file, output_yaml):
    result = {
        "name": "LDBC",
        "storeType": "gart",
        "xCsrParams": {"Ordering": "by_src"},
        "schema": {"vertexTypes": [], "edgeTypes": []},
    }

    with open(rgmapping_file, "r", encoding="UTF-8") as f:
        config = yaml.safe_load(f)

    vdefs = config["vertexMappings"]["vertex_types"]
    vtype_to_id = {}
    idx = 0
    for vdef in vdefs:
        id_name = vdef["idFieldName"]
        props = vdef["mappings"]
        type = vdef["type_name"]
        table_name = vdef["dataSourceName"]
        element = {"typeId": idx, "typeName": type}
        vtype_to_id[type] = idx

        p_idx = 0
        pelements = []
        for prop in props:
            p_name = prop["property"]
            col_name = prop["dataField"]["name"]
            pele = {"propertyId": p_idx, "propertyName": p_name}
            p_type = ""
            for key, value in schema[table_name]:
                if key == col_name:
                    p_type = value

            if p_type == "":
                print(
                    "Cannot find the type of property `%s` in table `%s`"
                    % (col_name, table_name)
                )
                exit(1)

            # TODO: fix the type to the unified format (DT_...)
            if p_name == id_name:
                pele["propertyType"] = {"primitiveType": p_type}
            else:
                pele["propertyType"] = p_type

            pelements.append(pele)
            p_idx += 1

        element["properties"] = pelements
        result["schema"]["vertexTypes"].append(element)
        idx += 1

    edefs = config["edgeMappings"]["edge_types"]
    idx = 0
    for edef in edefs:
        id_name = ""  # TODO: not support id in edge yet
        props = edef["dataFieldMappings"]
        type = edef["type_pair"]["edge"]
        table_name = edef["dataSourceName"]
        element = {"typeId": idx, "typeName": type}

        p_idx = 0
        pelements = []
        for prop in props:
            p_name = prop["property"]
            pele = {"propertyId": p_idx, "propertyName": p_name}
            p_type = ""
            for key, value in schema[table_name]:
                if key == p_name:
                    p_type = value

            # TODO: fix the type to the unified format (DT_...)
            if p_name == id_name:
                pele["propertyType"] = {"primitiveType": p_type}
            else:
                pele["propertyType"] = p_type

            pelements.append(pele)
            p_idx += 1

        element["properties"] = pelements

        relation = {}
        src_type = edef["type_pair"]["source_vertex"]
        dst_type = edef["type_pair"]["destination_vertex"]

        relation["srcTypeId"] = vtype_to_id[src_type]
        relation["dstTypeId"] = vtype_to_id[dst_type]

        # TODO: fix here
        relation["relation"] = "MANY_TO_MANY"

        element["vertexTypePairRelations"] = [relation]
        result["schema"]["edgeTypes"].append(element)
        idx += 1

    with open(output_yaml, "w", encoding="UTF-8") as f:
        yaml.dump(result, f, sort_keys=False)


if __name__ == "__main__":
    arg_parser = get_parser()
    args = arg_parser.parse_args()

    unset = False
    if not isinstance(args.user, str) or len(args.user) == 0:
        print("Please specify the database user with --user")
        unset = True

    if not isinstance(args.password, str) or len(args.password) == 0:
        print("Please specify the database password with --password")
        unset = True

    if not isinstance(args.db, str) or len(args.db) == 0:
        print("Please specify the database name with --password")
        unset = True

    if not isinstance(args.db_type, str) or args.db_type not in ["mysql", "postgresql"]:
        print("Please specify the database type with --db_type: mysql or postgresql")
        unset = True

    if unset:
        sys.exit(1)

    if args.db_type == "mysql":
        connection_string = "mysql+pymysql://%s:%s@%s:%s/%s" % (
            args.user,
            args.password,
            args.host,
            args.port,
            args.db,
        )
        engine = create_engine(connection_string, echo=False)
    elif args.db_type == "postgresql":
        connection_string = "postgresql://%s:%s@%s:%s/%s" % (
            args.user,
            args.password,
            args.host,
            args.port,
            args.db,
        )
        engine = create_engine(connection_string, echo=False)
    else:
        print("We now only support mysql and postgresql")
        exit(1)
    conn = engine.raw_connection()
    cursor = conn.cursor()

    schema, sum_row = exetract_schema(
        cursor, args.rgmapping_file, args.db, args.db_type, args.output
    )
    conn.close()

    produce_graph_schema(schema, args.rgmapping_file, args.output_yaml)

    print(sum_row)
