#!/usr/bin/env python3

import argparse
import sys
from sqlalchemy import create_engine


def get_parser():
    parser = argparse.ArgumentParser(
        description="Initialize the LDBC dataset in relational database",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--host", default="127.0.0.1", help="Database server host")
    parser.add_argument("--port", default=3306, help="Database server port")
    parser.add_argument("--user", help="Database user")
    parser.add_argument("--password", help="Database password")
    parser.add_argument("--db", default="ldbc", help="Database name")
    parser.add_argument(
        "--db_type", default="mysql", help="Which database to use, mysql or postgresql"
    )
    parser.add_argument(
        "--drop", type=bool, default=False, help="Drop the database and table if exists"
    )

    return parser


def get_args():
    args = get_parser().parse_args()

    unset = False

    if not isinstance(args.user, str) or len(args.user) == 0:
        print("Please specify the database user with --user")
        unset = True

    if not isinstance(args.password, str) or len(args.password) == 0:
        print("Please specify the database password with --password")
        unset = True

    if unset:
        sys.exit(1)

    return args


vertexTableDDL = {
    "organisation": """
        CREATE TABLE organisation (
            org_id BIGINT NOT NULL,
            org_type VARCHAR(255),
            org_name TEXT,
            org_url TEXT,
            PRIMARY KEY (org_id)
        )
    """,
    "place": """
        CREATE TABLE place (
            pla_id BIGINT NOT NULL,
            pla_name TEXT,
            pla_url TEXT,
            pla_type VARCHAR(255),
            PRIMARY KEY (pla_id)
        )
    """,
    "tag": """
        CREATE TABLE tag (
            tag_id BIGINT NOT NULL,
            tag_name TEXT,
            tag_url TEXT,
            PRIMARY KEY (tag_id)
        )
    """,
    "tagclass": """
        CREATE TABLE tagclass (
            tagc_id BIGINT NOT NULL,
            tagc_name TEXT,
            tagc_url TEXT,
            PRIMARY KEY (tagc_id)
        )
    """,
    "person": """
        CREATE TABLE person (
            p_id BIGINT NOT NULL,
            p_first_name VARCHAR(255),
            p_last_name VARCHAR(255),
            p_gender VARCHAR(255),
            p_birthday VARCHAR(255),
            p_creation_date VARCHAR(255),
            p_location_ip VARCHAR(255),
            p_browser_used VARCHAR(255),
            PRIMARY KEY (p_id)
        )
    """,
    "comment": """
        CREATE TABLE comment (
            co_id BIGINT NOT NULL,
            co_creation_date VARCHAR(255),
            co_location_ip VARCHAR(255),
            co_browser_used VARCHAR(255),
            co_content TEXT,
            co_length INT,
            PRIMARY KEY (co_id)
        )
    """,
    "post": """
        CREATE TABLE post (
            po_id BIGINT NOT NULL,
            po_image_file VARCHAR(255),
            po_creation_date VARCHAR(255),
            po_location_ip VARCHAR(255),
            po_browser_used VARCHAR(255),
            po_language VARCHAR(255),
            po_content TEXT,
            po_length INT,
            PRIMARY KEY (po_id)
        )
    """,
    "forum": """
        CREATE TABLE forum (
            fo_id BIGINT NOT NULL,
            fo_title TEXT,
            fo_creation_date VARCHAR(255),
            PRIMARY KEY (fo_id)
        )
    """,
}


# Function to generate edge table SQL based on a given table name
def generate_edge_ddl(table_name, additional_columns=""):
    return f"""
        CREATE TABLE {table_name} (
            src BIGINT,
            dst BIGINT,
            {additional_columns}
            PRIMARY KEY (src, dst)
        )
    """


# Dictionary of edge tables and their generated SQL statements
simepleEdgeTables = [
    "org_islocationin",
    "ispartof",
    "issubclassof",
    "hastype",
    "comment_hascreator",
    "comment_hastag",
    "comment_islocationin",
    "replyof_comment",
    "replyof_post",
    "post_hascreator",
    "post_hastag",
    "post_islocationin",
    "forum_containerof",
    "forum_hasmoderator",
    "forum_hastag",
    "person_hasinterest",
    "person_islocationin",
    "forum_hasmember",
]

simpleEdgeTableDDL = {
    table_name: generate_edge_ddl(table_name) for table_name in simepleEdgeTables
}

propertyEdgeTables = {
    "knows": "pk_creation_date VARCHAR(255),",
    "likes_comment": "likes_co_creation_date VARCHAR(255),",
    "likes_post": "likes_po_creation_date VARCHAR(255),",
    "studyat": "sa_class_year INT,",
    "workat": "wa_work_from INT,",
}

propertyEdgeTableDDL = {
    table_name: generate_edge_ddl(table_name, additional_columns=additional_columns)
    for table_name, additional_columns in propertyEdgeTables.items()
}


# create vertex tables
# Function to create a table
def create_table(cursor, table_name, sql_statement, vertex=True):
    cursor.execute(f"DROP TABLE IF EXISTS {table_name}")
    cursor.execute(sql_statement)
    prefix = "Created vertex" if vertex else "Created edge"
    print(f"{prefix} table: {table_name}")


def main():
    args = get_args()
    print("Args: ", args)
    print("\n========================================\n")

    if args.db_type == "mysql":
        connection_string = "mysql+pymysql://%s:%s@%s:%s/" % (
            args.user,
            args.password,
            args.host,
            args.port,
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

    # create the database
    if args.drop:
        print(f"Drop database {args.db}")
        if args.db_type == "mysql":
            cursor.execute(f"DROP DATABASE IF EXISTS {args.db}")
        elif args.db_type == "postgresql":
            print("Not supported drop database for postgresql.")

    if args.db_type == "mysql":
        cursor.execute(f"CREATE DATABASE IF NOT EXISTS {args.db}")
        cursor.execute(f"USE {args.db}")
    # Not supported create database for postgresql

    # create vertex tables
    for table_name, sql_statement in vertexTableDDL.items():
        create_table(cursor, table_name, sql_statement)

    # insert edge tables
    for table_name, sql_statement in simpleEdgeTableDDL.items():
        create_table(cursor, table_name, sql_statement, False)

    for table_name, sql_statement in propertyEdgeTableDDL.items():
        create_table(cursor, table_name, sql_statement, False)

    conn.commit()

    print("\n========================================\n")
    if args.db_type == "mysql":
        sql = f"SHOW TABLES FROM {args.db}"
        cursor.execute(sql)
        print("Execute SQL: " + sql)
        print("-----------------------------------------")
        results = cursor.fetchall()
        for row in results:
            print(row[0])
    elif args.db_type == "postgresql":
        cursor.execute(
            "SELECT table_name FROM information_schema.tables WHERE table_schema = 'public'"
        )
        # Fetch all the results
        results = cursor.fetchall()
        # Print the table names
        for row in results:
            print(row[0])

    print(f"Done creating {len(results)} tables")

    conn.close()


if __name__ == "__main__":
    main()
