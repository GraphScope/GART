#!/usr/bin/env python3

import argparse
import sys
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

base_dir = args.data_dir

print("01. Inserting organisation table...")
file_name = base_dir + "/organisation_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    header = f.readline()  # skip the header
    line = f.readline()
    num_lines = 0
    while line:
        org_id, org_type, org_name, org_url = line.strip().split("|")
        org_name = org_name.replace("'", "")
        org_url = org_url.replace("'", "")
        cursor.execute(
            f"insert into organisation values('{org_id}',"
            f"'{org_type}', '{org_name}', '{org_url}')"
        )
        line = f.readline()
        num_lines += 1
conn.commit()
print(f"01. Insert {num_lines} rows into organisation table")

print("02. Inserting place table...")
file_name = base_dir + "/place_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    header = f.readline()  # skip the header
    line = f.readline()
    num_lines = 0
    while line:
        pla_id, pla_name, pla_url, pla_type = line.strip().split("|")
        pla_name = pla_name.replace("'", "")
        pla_url = pla_url.replace("'", "")
        cursor.execute(
            f"insert into place values('{pla_id}',"
            f"'{pla_name}', '{pla_url}', '{pla_type}')"
        )
        line = f.readline()
        num_lines += 1
conn.commit()
print(f"02. Insert {num_lines} rows into place table")

print("03. Inserting tag table...")
file_name = base_dir + "/tag_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    header = f.readline()  # skip the header
    line = f.readline()
    num_lines = 0
    while line:
        tag_id, tag_name, tag_url = line.strip().split("|")
        tag_url = tag_url.replace("'", "")
        cursor.execute(f"insert into tag values('{tag_id}', '{tag_name}', '{tag_url}')")
        line = f.readline()
        num_lines += 1
conn.commit()
print(f"03. Insert {num_lines} rows into tag table")

print("04. Inserting tagclass table...")
file_name = base_dir + "/tagclass_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    header = f.readline()  # skip the header
    line = f.readline()
    num_lines = 0
    while line:
        tagc_id, tagc_name, tagc_url = line.strip().split("|")
        tagc_url = tagc_url.replace("'", "")
        cursor.execute(
            f"insert into tagclass values('{tagc_id}', '{tagc_name}', '{tagc_url}')"
        )
        line = f.readline()
        num_lines += 1
conn.commit()
print(f"04. Insert {num_lines} rows into tagclass table")

print("05. Inserting person table...")
file_name = base_dir + "/person_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    header = f.readline()  # skip the header
    line = f.readline()
    num_lines = 0
    while line:
        (
            p_id,
            p_first_name,
            p_last_name,
            p_gender,
            p_birthday,
            p_creation_date,
            p_location_ip,
            p_browser_used,
        ) = line.strip().split("|")
        cursor.execute(
            f"insert into person values('{p_id}', "
            f"'{p_first_name}', '{p_last_name}', '{p_gender}', "
            f"'{p_birthday}', '{p_creation_date}', "
            f"'{p_location_ip}', '{p_browser_used}')"
        )
        line = f.readline()
        num_lines += 1
conn.commit()
print(f"05. Insert {num_lines} rows into person table")

print("06. Inserting comment table...")
file_name = base_dir + "/comment_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    header = f.readline()  # skip the header
    line = f.readline()
    num_lines = 0
    while line:
        (
            co_id,
            co_creation_date,
            co_location_ip,
            co_browser_used,
            co_content,
            co_length,
        ) = line.strip().split("|")
        co_content = co_content.replace("'", "")
        cursor.execute(
            f"insert into comment values('{co_id}', "
            f"'{co_creation_date}', '{co_location_ip}', "
            f"'{co_browser_used}', '{co_content}', {co_length})"
        )
        line = f.readline()
        num_lines += 1
conn.commit()
print(f"06. Insert {num_lines} rows into comment table")

print("07. Inserting post table...")
file_name = base_dir + "/post_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    header = f.readline()  # skip the header
    line = f.readline()
    num_lines = 0
    while line:
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
        po_content = po_content.replace("'", "")
        cursor.execute(
            f"insert into post values('{po_id}', "
            f"'{po_image_file}', '{po_creation_date}', "
            f"'{po_location_ip}', '{po_browser_used}', "
            f"'{po_language}', '{po_content}', {po_length})"
        )
        line = f.readline()
        num_lines += 1
conn.commit()
print(f"07. Insert {num_lines} rows into post table")

print("08. Inserting forum table...")
file_name = base_dir + "/forum_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    header = f.readline()  # skip the header
    line = f.readline()
    num_lines = 0
    while line:
        fo_id, fo_title, fo_creation_date = line.strip().split("|")
        fo_title = fo_title.replace("'", "")
        cursor.execute(
            f"insert into forum values('{fo_id}', "
            f"'{fo_title}', '{fo_creation_date}')"
        )
        line = f.readline()
        num_lines += 1
conn.commit()
print(f"08. Insert {num_lines} rows into forum table")

# for buldload test
print("Input any key to insert edge tables...")
content = sys.stdin.readline()
print("Inserting edge tables...", content)

# insert edge tables


def insert_simple_edges(prefix, csv_file, table_name):
    print(f"{prefix}. Inserting {table_name} table...")
    file = base_dir + csv_file
    with open(file, "r", encoding="UTF-8") as f:
        header = f.readline()  # skip the header
        line = f.readline()
        num_lines = 0
        while line:
            src, dst = line.strip().split("|")
            cursor.execute(f"insert into {table_name} values('{src}', '{dst}')")
            line = f.readline()
            num_lines += 1
    conn.commit()
    print(f"{prefix}. Insert {num_lines} rows into {table_name} table")


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


def insert_prop_edges(prefix, csv_file, table_name):
    print(f"{prefix}. Inserting {table_name} table...")
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
    print(f"{prefix}. Insert {num_lines} rows into {table_name} table")


insert_prop_edges("26", "/forum_hasMember_person_0_0.csv", "forum_hasmember")

insert_prop_edges("27", "/person_knows_person_0_0.csv", "knows")

insert_prop_edges("28", "/person_likes_comment_0_0.csv", "likes_comment")

insert_prop_edges("29", "/person_likes_post_0_0.csv", "likes_post")

insert_prop_edges("30", "/person_studyAt_organisation_0_0.csv", "studyat")

insert_prop_edges("31", "/person_workAt_organisation_0_0.csv", "workat")

conn.close()
