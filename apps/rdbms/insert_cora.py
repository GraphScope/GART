#!/usr/bin/env python3

import argparse
import time
import subprocess
import sys
import os

from psycopg2 import errors
from sqlalchemy import create_engine


def get_parser():
    parser = argparse.ArgumentParser(
        description="Insert Cora dataset into relational database",
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
    parser.add_argument("--data_dir", help="Dataset directory")
    parser.add_argument(
        "--init", type=bool, default=False, help="Initialize the database"
    )

    return parser


arg_parser = get_parser()
args = arg_parser.parse_args()
unset = False

if not isinstance(args.data_dir, str) or len(args.data_dir) == 0:
    print("Please specify the Cora dataset directory with --data_dir")
    unset = True

if not isinstance(args.user, str) or len(args.user) == 0:
    print("Please specify the MySQL user with --user")
    unset = True

if not isinstance(args.password, str) or len(args.password) == 0:
    print("Please specify the MySQL password with --password")
    unset = True

if unset:
    sys.exit(1)

print("Args: ", args)

if args.init:
    current_file_path = os.path.abspath(__file__)
    current_directory = os.path.dirname(current_file_path)
    init_path = os.path.join(current_directory, "init_cora_schema.py")

    init_args = [
        "--host",
        args.host,
        "--port",
        str(args.port),
        "--user",
        args.user,
        "--password",
        args.password,
        "--db",
        args.db,
        "--db_type",
        args.db_type,
    ]
    command = ["python", init_path] + init_args
    subprocess.run(command, check=True)


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


class Timer:
    def __init__(self):
        self.start_time = None
        self.end_time = None
        self.duration = None
        self.total_time = 0.0

    def start(self):
        self.start_time = time.time()

    def end(self):
        if self.start_time is None:
            print("Please call the start() method to record the start time first.")
        else:
            self.end_time = time.time()
            self.duration = self.end_time - self.start_time
            self.total_time += self.duration

    def interval(self):
        if self.start_time is None or self.end_time is None:
            print(
                "Please call the start() and end() methods to record the time interval first."
            )
        else:
            return self.duration

    def total(self):
        return self.total_time

    def unit(self):
        return "seconds"

    def print(self):
        if self.start_time is None or self.end_time is None:
            print(
                "Please call the start() and end() methods to record the time interval first."
            )
        else:
            duration = self.end_time - self.start_time
            print("Time interval: ：{:.2f} second".format(duration))


timer = Timer()
conn = engine.raw_connection()
cursor = conn.cursor()
base_dir = args.data_dir


# `process_line_func`: func(line) -> sql
def insert_vertices(prefix, csv_file, table_name, process_line_func):
    # print(f"{prefix}. Inserting {table_name} table...")
    timer.start()
    file = base_dir + csv_file
    with open(file, "r", encoding="UTF-8") as f:
        # f.readline()  # skip the header
        line = f.readline()
        num_lines = 0
        while line:
            sql = process_line_func(line)
            try:
                cursor.execute(sql)
            except errors.SyntaxError as error:
                print("SyntaxError occurred:", error)
                print(sql)
            line = f.readline()
            num_lines += 1
    conn.commit()
    timer.end()
    formatted_interval = "{:.2f}".format(timer.interval())
    print(
        f"{prefix}. Insert {num_lines} rows into {table_name} table, time: {formatted_interval} {timer.unit()}"
    )


def insert_simple_edges(prefix, csv_file, table_name):
    # print(f"{prefix}. Inserting {table_name} table...")
    timer.start()
    file = base_dir + csv_file
    with open(file, "r", encoding="UTF-8") as f:
        # header = f.readline()  # skip the header
        line = f.readline()
        num_lines = 0
        while line:
            src, dst = line.strip().split()
            cursor.execute(f"insert into {table_name} values({src}, {dst})")
            line = f.readline()
            num_lines += 1
    conn.commit()
    timer.end()
    formatted_interval = "{:.2f}".format(timer.interval())
    print(
        f"{prefix}. Insert {num_lines} rows into {table_name} table, time: {formatted_interval} {timer.unit()}"
    )


def insert_prop_edges(prefix, csv_file, table_name):
    # print(f"{prefix}. Inserting {table_name} table...")
    timer.start()
    file = base_dir + csv_file
    with open(file, "r", encoding="UTF-8") as f:
        header = f.readline()  # skip the header
        line = f.readline()
        num_lines = 0
        while line:
            src, dst, prop = line.strip().split("|")
            cursor.execute(
                f"insert into {table_name} values('{src}', '{dst}', '{prop}')"
            )
            line = f.readline()
            num_lines += 1
    conn.commit()
    timer.end()
    formatted_interval = "{:.2f}".format(timer.interval())
    print(
        f"{prefix}. Insert {num_lines} rows into {table_name} table, time: {formatted_interval} {timer.unit()}"
    )


# Insert vertex tables


# 01. paper
def process_paper(line):
    data = line.strip().split()
    paper_id = data[0]
    paper_feat_0 = data[0]
    paper_feat_1 = data[0]
    paper_feat_2 = data[0]
    paper_feat_3 = data[0]
    sql = (
        f"insert into paper values({paper_id}, {paper_feat_0}, "
        f"{paper_feat_1}, {paper_feat_2}, {paper_feat_3})"
    )
    return sql


insert_vertices("01", "/cora.content", "paper", process_paper)

# insert edge tables

insert_simple_edges("25", "/cora.cites", "paper_cites_paper")


conn.close()

print("Total time: {:.2f} {}".format(timer.total(), timer.unit()))
