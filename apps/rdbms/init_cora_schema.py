#!/usr/bin/env python3

import argparse
import sys
from sqlalchemy import create_engine


def get_parser():
    parser = argparse.ArgumentParser(
        description="Initialize the Cora dataset in relational database",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--host", default="127.0.0.1", help="Database server host")
    parser.add_argument("--port", default=3306, help="Database server port")
    parser.add_argument("--user", help="Database user")
    parser.add_argument("--password", help="Database password")
    parser.add_argument("--db", default="cora", help="Database name")
    parser.add_argument(
        "--db_type", default="mysql", help="Which database to use, mysql or postgresql"
    )
    parser.add_argument(
        "--drop", type=bool, default=False, help="Drop the database and table if exists"
    )

    return parser


arg_parser = get_parser()
args = arg_parser.parse_args()

unset = False

if not isinstance(args.user, str) or len(args.user) == 0:
    print("Please specify the database user with --user")
    unset = True

if not isinstance(args.password, str) or len(args.password) == 0:
    print("Please specify the database password with --password")
    unset = True

if unset:
    sys.exit(1)

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
cursor.execute("DROP TABLE IF EXISTS paper")
sql = """CREATE TABLE paper (
        paper_id INT NOT NULL,
        paper_feat_0 FLOAT,
        paper_feat_1 FLOAT,
        paper_feat_2 FLOAT,
        paper_feat_3 FLOAT )"""
cursor.execute(sql)

print("Created vertex table: paper")

# insert edge tables

cursor.execute("DROP TABLE IF EXISTS paper_cites_paper")
sql = """CREATE TABLE paper_cites_paper (
        src INT,
        dst INT
        )"""
cursor.execute(sql)

print("Created edge table: paper_cites_paper")

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
