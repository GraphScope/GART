#!/usr/bin/env python3

import argparse
import multiprocessing
import subprocess
import sys
import time
import os

from psycopg2 import errors
from sqlalchemy import create_engine


def get_parser():
    parser = argparse.ArgumentParser(
        description="Insert LDBC dataset into relational database",
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
    parser.add_argument("--data_dir", help="LDBC dataset directory (dynamic)")
    parser.add_argument(
        "--init", type=bool, default=False, help="Initialize the database"
    )

    return parser


arg_parser = get_parser()
args = arg_parser.parse_args()
unset = False

if not isinstance(args.data_dir, str) or len(args.data_dir) == 0:
    print("Please specify the LDBC dataset directory with --data_dir")
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
    init_path = os.path.join(current_directory, "init_schema.py")

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
            print("Time interval: {:.2f} second".format(duration))


total_timer = Timer()
base_dir = args.data_dir
vertex_process = []
edge_process = []


# `process_line_func`: func(line) -> sql
def insert_vertices_thread(prefix, csv_file, table_name, process_line_func):
    # print(f"{prefix}. Inserting {table_name} table...")
    conn = engine.raw_connection()
    cursor = conn.cursor()
    sql = ""
    cursor.execute(f"DELETE FROM {table_name};")  # TODO(SSJ): checkpoint?

    timer = Timer()
    timer.start()
    file = base_dir + csv_file
    with open(file, "r", encoding="UTF-8") as f:
        f.readline()  # skip the header
        line = f.readline()
        num_lines = 0
        batch_size = 10000  # Number of records to insert in each batch
        batch = []
        while line:
            result = process_line_func(line)
            batch.append(result)
            if len(batch) >= batch_size:
                try:
                    sql = (
                        f"insert into {table_name} values ("
                        + "%s," * (len(batch[0]) - 1)
                        + "%s)"
                    )
                    cursor.executemany(sql, batch)
                    batch = []
                    conn.commit()
                except errors.SyntaxError as error:
                    print("SyntaxError occurred:", error)
                    print(sql)
            line = f.readline()
            num_lines += 1
    if batch:
        try:
            sql = (
                f"insert into {table_name} values ("
                + "%s," * (len(batch[0]) - 1)
                + "%s)"
            )
            cursor.executemany(sql, batch)
            conn.commit()
        except errors.SyntaxError as error:
            print("SyntaxError occurred:", error)
            print(sql)

    timer.end()
    formatted_interval = "{:.2f}".format(timer.interval())
    print(
        f"{prefix}. Insert {num_lines} rows into {table_name} table, time: {formatted_interval} {timer.unit()}"
    )
    conn.close()


def insert_vertices(prefix, csv_file, table_name, process_line_func):
    prcoess = multiprocessing.Process(
        target=insert_vertices_thread,
        args=(prefix, csv_file, table_name, process_line_func),
    )
    vertex_process.append(prcoess)
    prcoess.start()


def insert_simple_edges_thread(prefix, csv_file, table_name):
    # print(f"{prefix}. Inserting {table_name} table...")
    conn = engine.raw_connection()
    cursor = conn.cursor()
    sql = ""
    cursor.execute(f"DELETE FROM {table_name};")  # TODO(SSJ): checkpoint?

    timer = Timer()
    timer.start()
    file = base_dir + csv_file
    if not os.path.isfile(file):
        lower_file = base_dir + csv_file.lower()
        if not os.path.isfile(lower_file):
            print(f"Files {file} and {lower_file} do not exist.")
        file = lower_file

    with open(file, "r", encoding="UTF-8") as f:
        header = f.readline()  # skip the header
        line = f.readline()
        num_lines = 0
        batch_size = 10000  # Number of records to insert in each batch
        batch = []
        while line:
            src, dst = line.strip().split("|")
            batch.append((src, dst))
            if len(batch) >= batch_size:
                sql = (
                    f"insert into {table_name} values ("
                    + "%s," * (len(batch[0]) - 1)
                    + "%s)"
                )
                cursor.executemany(sql, batch)
                conn.commit()
                batch = []
            line = f.readline()
            num_lines += 1
    if batch:
        sql = f"insert into {table_name} values (" + "%s," * (len(batch[0]) - 1) + "%s)"
        cursor.executemany(sql, batch)
        conn.commit()

    timer.end()
    formatted_interval = "{:.2f}".format(timer.interval())
    print(
        f"{prefix}. Insert {num_lines} rows into {table_name} table, time: {formatted_interval} {timer.unit()}"
    )
    conn.close()


def insert_simple_edges(prefix, csv_file, table_name):
    process = multiprocessing.Process(
        target=insert_simple_edges_thread, args=(prefix, csv_file, table_name)
    )
    edge_process.append(process)
    process.start()


def insert_prop_edges_thread(prefix, csv_file, table_name):
    # print(f"{prefix}. Inserting {table_name} table...")
    conn = engine.raw_connection()
    cursor = conn.cursor()
    sql = ""
    cursor.execute(f"DELETE FROM {table_name};")  # TODO(SSJ): checkpoint?

    timer = Timer()
    timer.start()
    file = base_dir + csv_file
    if not os.path.isfile(file):
        lower_file = base_dir + csv_file.lower()
        if not os.path.isfile(lower_file):
            print(f"Files {file} and {lower_file} do not exist.")
        file = lower_file

    with open(file, "r", encoding="UTF-8") as f:
        header = f.readline()  # skip the header
        line = f.readline()
        num_lines = 0
        batch_size = 10000  # Number of records to insert in each batch
        batch = []
        while line:
            try:
                src, dst, prop = line.strip().split("|")
            except ValueError:
                # print(f"{num_lines} line in file {csv_file} has error: {line}")
                src, dst = line.strip().split("|")
                prop = "2010-08-14T20:59:58.658+00:00"  # TODO(SSJ): for GIE
            batch.append((src, dst, prop))
            if len(batch) >= batch_size:
                sql = (
                    f"insert into {table_name} values ("
                    + "%s," * (len(batch[0]) - 1)
                    + "%s)"
                )
                cursor.executemany(sql, batch)
                conn.commit()
                batch = []
            line = f.readline()
            num_lines += 1
    if batch:
        sql = f"insert into {table_name} values (" + "%s," * (len(batch[0]) - 1) + "%s)"
        cursor.executemany(sql, batch)
        conn.commit()

    timer.end()
    formatted_interval = "{:.2f}".format(timer.interval())
    print(
        f"{prefix}. Insert {num_lines} rows into {table_name} table, time: {formatted_interval} {timer.unit()}"
    )
    conn.close()


def insert_prop_edges(prefix, csv_file, table_name):
    process = multiprocessing.Process(
        target=insert_prop_edges_thread, args=(prefix, csv_file, table_name)
    )
    edge_process.append(process)
    process.start()


# Insert vertex tables


# 01. organisation
def process_organisation(line):
    org_id, org_type, org_name, org_url = line.strip().split("|")
    org_name = org_name.replace("'", "''")
    org_url = org_url.replace("'", "''")
    result = (org_id, org_type, org_name, org_url)
    return result


# 02. place
def process_place(line):
    pla_id, pla_name, pla_url, pla_type = line.strip().split("|")
    pla_name = pla_name.replace("'", "''")
    pla_url = pla_url.replace("'", "''")
    result = (pla_id, pla_name, pla_url, pla_type)
    return result


# 03. tag
def process_tag(line):
    tag_id, tag_name, tag_url = line.strip().split("|")
    tag_url = tag_url.replace("'", "''")
    result = (tag_id, tag_name, tag_url)
    return result


# 04. tagclass
def process_tagclass(line):
    tagc_id, tagc_name, tagc_url = line.strip().split("|")
    tagc_url = tagc_url.replace("'", "''")
    result = (tagc_id, tagc_name, tagc_url)
    return result


# 05. person
def process_person(line):
    (
        p_id,
        p_first_name,
        p_last_name,
        p_gender,
        p_birthday,
        p_creation_date,
        p_location_ip,
        p_browser_used,
    ) = line.strip().split("|")[:8]
    p_first_name = p_first_name.replace("'", "''")
    p_last_name = p_last_name.replace("'", "''")
    result = (
        p_id,
        p_first_name,
        p_last_name,
        p_gender,
        p_birthday,
        p_creation_date,
        p_location_ip,
        p_browser_used,
    )
    return result


# 06. comment
def process_comment(line):
    (
        co_id,
        co_creation_date,
        co_location_ip,
        co_browser_used,
        co_content,
        co_length,
    ) = line.strip().split("|")
    co_content = co_content.replace("'", "''")
    result = (
        co_id,
        co_creation_date,
        co_location_ip,
        co_browser_used,
        co_content,
        co_length,
    )
    return result


# 07. post
def process_post(line):
    (
        po_id,
        po_image_file,
        po_creation_date,
        po_location_ip,
        po_browser_used,
        po_language,
        po_content,
        po_length,
    ) = line.strip().split("|")
    po_content = po_content.replace("'", "''")
    result = (
        po_id,
        po_image_file,
        po_creation_date,
        po_location_ip,
        po_browser_used,
        po_language,
        po_content,
        po_length,
    )
    return result


# 08. forum
def process_forum(line):
    fo_id, fo_title, fo_creation_date = line.strip().split("|")
    fo_title = fo_title.replace("'", "''")
    result = (fo_id, fo_title, fo_creation_date)
    return result


total_timer = Timer()
total_timer.start()

if True:
    insert_vertices("01", "/organisation_0_0.csv", "organisation", process_organisation)

    insert_vertices("02", "/place_0_0.csv", "place", process_place)

    insert_vertices("03", "/tag_0_0.csv", "tag", process_tag)

    insert_vertices("04", "/tagclass_0_0.csv", "tagclass", process_tagclass)

    insert_vertices("05", "/person_0_0.csv", "person", process_person)

    insert_vertices("06", "/comment_0_0.csv", "comment", process_comment)

    insert_vertices("07", "/post_0_0.csv", "post", process_post)

    insert_vertices("08", "/forum_0_0.csv", "forum", process_forum)

    for process in vertex_process:
        process.join()

    total_timer.end()
    print("Load vertex time: {:.2f} {}".format(total_timer.total(), total_timer.unit()))

    # insert edge tables without additional properties


    insert_simple_edges("09", "/organisation_isLocatedIn_place_0_0.csv", "org_islocationin")

    insert_simple_edges("10", "/place_isPartOf_place_0_0.csv", "ispartof")

    insert_simple_edges("11", "/tagclass_isSubclassOf_tagclass_0_0.csv", "issubclassof")

    insert_simple_edges("12", "/tag_hasType_tagclass_0_0.csv", "hastype")

    insert_simple_edges("13", "/comment_hasCreator_person_0_0.csv", "comment_hascreator")

    insert_simple_edges("14", "/comment_hasTag_tag_0_0.csv", "comment_hastag")

    insert_simple_edges("15", "/comment_isLocatedIn_place_0_0.csv", "comment_islocationin")

    insert_simple_edges("16", "/comment_replyOf_comment_0_0.csv", "replyof_comment")

    insert_simple_edges("17", "/comment_replyOf_post_0_0.csv", "replyof_post")

    insert_simple_edges("18", "/post_hasCreator_person_0_0.csv", "post_hascreator")

    insert_simple_edges("19", "/post_hasTag_tag_0_0.csv", "post_hastag")

    insert_simple_edges("20", "/post_isLocatedIn_place_0_0.csv", "post_islocationin")

    insert_simple_edges("21", "/forum_containerOf_post_0_0.csv", "forum_containerof")

    insert_simple_edges("22", "/forum_hasModerator_person_0_0.csv", "forum_hasmoderator")

    insert_simple_edges("23", "/forum_hasTag_tag_0_0.csv", "forum_hastag")
    
    insert_simple_edges("24", "/person_hasInterest_tag_0_0.csv", "person_hasinterest")

    insert_simple_edges("25", "/person_isLocatedIn_place_0_0.csv", "person_islocationin")

    # insert edge tables with additional properties

    insert_prop_edges("26", "/forum_hasMember_person_0_0.csv", "forum_hasmember")

if True:
    insert_prop_edges("27", "/person_knows_person_0_0.csv", "knows")

    insert_prop_edges("28", "/person_likes_comment_0_0.csv", "likes_comment")

    insert_prop_edges("29", "/person_likes_post_0_0.csv", "likes_post")

    insert_prop_edges("30", "/person_studyAt_organisation_0_0.csv", "studyat")

    insert_prop_edges("31", "/person_workAt_organisation_0_0.csv", "workat")

for process in edge_process:
    process.join()

total_timer.end()
print("Total time: {:.2f} {}".format(total_timer.total(), total_timer.unit()))
