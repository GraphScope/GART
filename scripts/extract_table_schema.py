#!/usr/bin/env python3

import sys

import argparse
import json
from sqlalchemy import create_engine
import yaml
import etcd3


def get_parser():
    parser = argparse.ArgumentParser(
        description="Launch database schema extractor",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--host", default="127.0.0.1", help="Database server host")
    parser.add_argument("--port", default=3306, help="Database server port")
    parser.add_argument("--user", help="Database user")
    parser.add_argument("--password", help="Database password")
    parser.add_argument("--etcd_endpoint", help="Etcd endpoint")
    parser.add_argument("--etcd_prefix", default="", help="Etcd prefix")
    parser.add_argument("--db", default="ldbc", help="Database name")
    parser.add_argument(
        "--db_type", default="mysql", help="Database type: mysql, postgresql"
    )

    parser.add_argument(
        "--rgmapping_file",
        default="schema/rgmapping-ldbc.yaml",
        help="Config file (RGMapping)",
    )

    return parser


class GSchemaLoader(yaml.SafeLoader):
    def let_it_through(self, node):
        return self.construct_mapping(node)


# For the YAML generated by JAVA code

GSchemaLoader.add_constructor(
    "tag:yaml.org,2002:gart.pgql.GSchema", GSchemaLoader.let_it_through
)


def exetract_schema(cursor, rgmapping_file, database, db_type, etcd_endpoint, etcd_prefix):
    with open(rgmapping_file, "r", encoding="UTF-8") as f:
        config = yaml.load(f, Loader=GSchemaLoader)
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
        
    index = etcd_endpoint.find(":")
    etcd_host = etcd_endpoint[:index]
    etcd_port = int(etcd_endpoint[index+1:])
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
    etcd_client.put(etcd_prefix + 'gart_table_schema', json.dumps(schema))

    return schema, sum_row


# schema: {table_name: [(column_name, column_type), ...]}
def produce_graph_schema(schema, rgmapping_file, etcd_endpoint, etcd_prefix):
    index = etcd_endpoint.find(":")
    etcd_host = etcd_endpoint[:index]
    etcd_port = int(etcd_endpoint[index+1:])
    etcd_client = etcd3.client(host=etcd_host, port=etcd_port)
    result = {
        "name": "LDBC",
        "storeType": "gart",
        "xCsrParams": {"Ordering": "by_src"},
        "schema": {"vertexTypes": [], "edgeTypes": []},
    }

    with open(rgmapping_file, "r", encoding="UTF-8") as f:
        config = yaml.load(f, Loader=GSchemaLoader)
        etcd_client.put(etcd_prefix + 'gart_rg_mapping_yaml', yaml.dump(config, sort_keys=False))

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
        
    etcd_client.put(etcd_prefix + 'gart_graph_schema_yaml', yaml.dump(result, sort_keys=False))


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
    
    if not isinstance(args.etcd_endpoint, str) or len(args.etcd_endpoint) == 0:
        print("Please specify the etcd endpoint with --etcd_endpoint")
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
        cursor, args.rgmapping_file, args.db, args.db_type, args.etcd_endpoint, args.etcd_prefix
    )
    conn.close()

    produce_graph_schema(schema, args.rgmapping_file, args.etcd_endpoint, args.etcd_prefix)

    print(sum_row)
