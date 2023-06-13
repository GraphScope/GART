#!/usr/bin/env python3

import sys

import argparse
import json
import pymysql
import yaml


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


def exetract_schema(cursor, rgmapping_file, database, output):
    with open(rgmapping_file, "r", encoding="UTF-8") as f:
        config = yaml.safe_load(f)
    tables = (
        config["vertexMappings"]["vertex_types"] + config["edgeMappings"]["edge_types"]
    )
    schema = {}
    for table in tables:
        table_name = table["dataSourceName"]
        # sql = "SHOW COLUMNS FROM " + table_name
        sql = f'''SELECT COLUMN_NAME, COLUMN_TYPE
                FROM information_schema.COLUMNS
                WHERE TABLE_NAME="{table_name}" and TABLE_SCHEMA="{database}"'''
        cursor.execute(sql)
        results = cursor.fetchall()
        schema[table_name] = results
    with open(output, "w", encoding="UTF-8") as f:
        json.dump(schema, f, indent=4)

    return schema


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
        element = {"typeId": idx, "typeName": type}
        vtype_to_id[type] = idx

        p_idx = 0
        pelements = []
        for prop in props:
            p_name = prop["property"]
            pele = {"propertyId": p_idx, "propertyName": p_name}
            p_type = ""
            for key, value in schema[type]:
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
        result["schema"]["vertexTypes"].append(element)
        idx += 1

    edefs = config["edgeMappings"]["edge_types"]
    idx = 0
    for edef in edefs:
        props = edef["dataFieldMappings"]
        type = edef["type_pair"]["edge"]
        element = {"typeId": idx, "typeName": type}

        p_idx = 0
        pelements = []
        for prop in props:
            p_name = prop["property"]
            pele = {"propertyId": p_idx, "propertyName": p_name}
            p_type = ""
            for key, value in schema[type]:
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

    with open(output_yaml, "w") as f:
        yaml.dump(result, f, sort_keys=False)


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

    db = pymysql.connect(
        host=args.host,
        port=int(args.port),
        user=args.user,
        password=args.password,
        database=args.db,
    )
    db_cursor = db.cursor()
    schema = exetract_schema(db_cursor, args.rgmapping_file, args.db, args.output)
    db_cursor.close()

    produce_graph_schema(schema, args.rgmapping_file, args.output_yaml)
