#!/usr/bin/env python3

import argparse
import sys
import pymysql

def get_parser():
    parser = argparse.ArgumentParser(
        description="Initialize the LDBC dataset in MySQL",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("--host", default="127.0.0.1", help="MySQL host")
    parser.add_argument("--port", default=3306, help="MySQL port")
    parser.add_argument("--user", help="MySQL user")
    parser.add_argument("--password", help="MySQL password")
    parser.add_argument("--db", default="my_maxwell_01",
                        help="MySQL database")
    parser.add_argument("--drop", type=bool, default=False,
                        help="Drop the database and table if exists")

    return parser

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

db = pymysql.connect(host=args.host,
                     port=args.port,
                     user=args.user,
                     password=args.password)
cursor = db.cursor()

# create the database
if args.drop:
    print(f"Drop database {args.db}")
    cursor.execute(f"DROP DATABASE IF EXISTS {args.db}")
cursor.execute(f"CREATE DATABASE IF NOT EXISTS {args.db}")
cursor.execute(f"USE {args.db}")

# create vertex tables
cursor.execute("DROP TABLE IF EXISTS organisation")
sql = """CREATE TABLE organisation (
         EPOCH VARCHAR(255) NOT NULL,
         org_id VARCHAR(255) NOT NULL,
         org_type VARCHAR(255),
         org_name TEXT,
         org_url TEXT )"""
cursor.execute(sql)

print("Created vertex table: organisation")

cursor.execute("DROP TABLE IF EXISTS place")
sql = """CREATE TABLE place (
         EPOCH VARCHAR(255) NOT NULL,
         pla_id VARCHAR(255) NOT NULL,
         pla_name TEXT,
         pla_url TEXT,
         pla_type VARCHAR(255) )"""
cursor.execute(sql)

print("Created vertex table: place")

cursor.execute("DROP TABLE IF EXISTS tag")
sql = """CREATE TABLE tag (
         EPOCH VARCHAR(255) NOT NULL,
         tag_id VARCHAR(255) NOT NULL,
         tag_name TEXT,
         tag_url TEXT)"""
cursor.execute(sql)

print("Created vertex table: tag")

cursor.execute("DROP TABLE IF EXISTS tagclass")
sql = """CREATE TABLE tagclass (
         EPOCH VARCHAR(255) NOT NULL,
         tagc_id VARCHAR(255) NOT NULL,
         tagc_name TEXT,
         tagc_url TEXT)"""
cursor.execute(sql)

print("Created vertex table: tagclass")

cursor.execute("DROP TABLE IF EXISTS person")
sql = """CREATE TABLE person (
         EPOCH VARCHAR(255) NOT NULL,
         p_id VARCHAR(255) NOT NULL,
         p_first_name VARCHAR(255),
         p_last_name VARCHAR(255),
         p_gender VARCHAR(255),
         p_birthday VARCHAR(255),
         p_creation_date VARCHAR(255),
         p_location_ip VARCHAR(255),
         p_browser_used VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created vertex table: person")

cursor.execute("DROP TABLE IF EXISTS comment")
sql = """CREATE TABLE comment (
         EPOCH VARCHAR(255) NOT NULL,
         co_id VARCHAR(255) NOT NULL,
         co_creation_date VARCHAR(255),
         co_location_ip VARCHAR(255),
         co_browser_used VARCHAR(255),
         co_content TEXT,
         co_length INT
         )"""
cursor.execute(sql)

print("Created vertex table: comment")

cursor.execute("DROP TABLE IF EXISTS post")
sql = """CREATE TABLE post (
         EPOCH VARCHAR(255) NOT NULL,
         po_id VARCHAR(255) NOT NULL,
         po_image_file VARCHAR(255),
         po_creation_date VARCHAR(255),
         po_location_ip VARCHAR(255),
         po_browser_used VARCHAR(255),
         po_language VARCHAR(255),
         po_content TEXT,
         po_length INT
         )"""
cursor.execute(sql)

print("Created vertex table: post")

cursor.execute("DROP TABLE IF EXISTS forum")
sql = """CREATE TABLE forum (
         EPOCH VARCHAR(255) NOT NULL,
         fo_id VARCHAR(255) NOT NULL,
         fo_title TEXT,
         fo_creation_date VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created vertex table: forum")

# insert edge tables

cursor.execute("DROP TABLE IF EXISTS org_islocationin")
sql = """CREATE TABLE org_islocationin (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: org_islocationin")

cursor.execute("DROP TABLE IF EXISTS ispartof")
sql = """CREATE TABLE ispartof (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: ispartof")

cursor.execute("DROP TABLE IF EXISTS issubclassof")
sql = """CREATE TABLE issubclassof (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: issubclassof")

cursor.execute("DROP TABLE IF EXISTS hastype")
sql = """CREATE TABLE hastype (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: hastype")

cursor.execute("DROP TABLE IF EXISTS comment_hascreator")
sql = """CREATE TABLE comment_hascreator (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: comment_hascreator")

cursor.execute("DROP TABLE IF EXISTS comment_hastag")
sql = """CREATE TABLE comment_hastag (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: comment_hastag")

cursor.execute("DROP TABLE IF EXISTS comment_islocationin")
sql = """CREATE TABLE comment_islocationin (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: comment_islocationin")

cursor.execute("DROP TABLE IF EXISTS replyof_comment")
sql = """CREATE TABLE replyof_comment (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: replyof_comment")

cursor.execute("DROP TABLE IF EXISTS replyof_post")
sql = """CREATE TABLE replyof_post (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: replyof_post")

cursor.execute("DROP TABLE IF EXISTS post_hascreator")
sql = """CREATE TABLE post_hascreator (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: post_hascreator")

cursor.execute("DROP TABLE IF EXISTS post_hastag")
sql = """CREATE TABLE post_hastag (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: post_hastag")

cursor.execute("DROP TABLE IF EXISTS post_islocationin")
sql = """CREATE TABLE post_islocationin (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: post_islocationin")

cursor.execute("DROP TABLE IF EXISTS forum_containerof")
sql = """CREATE TABLE forum_containerof (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: forum_containerof")

cursor.execute("DROP TABLE IF EXISTS forum_hasmoderator")
sql = """CREATE TABLE forum_hasmoderator (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: forum_hasmoderator")

cursor.execute("DROP TABLE IF EXISTS forum_hastag")
sql = """CREATE TABLE forum_hastag (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: forum_hastag")

cursor.execute("DROP TABLE IF EXISTS person_hasinterest")
sql = """CREATE TABLE person_hasinterest (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: person_hasinterest")

cursor.execute("DROP TABLE IF EXISTS person_islocationin")
sql = """CREATE TABLE person_islocationin (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: person_islocationin")

cursor.execute("DROP TABLE IF EXISTS forum_hasmember")
sql = """CREATE TABLE forum_hasmember (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255),
         fo_hm_join_data VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: forum_hasmember")

cursor.execute("DROP TABLE IF EXISTS knows")
sql = """CREATE TABLE knows (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255),
         kn_creation_data VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: knows")

cursor.execute("DROP TABLE IF EXISTS likes_comment")
sql = """CREATE TABLE likes_comment (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255),
         likes_co_creation_data VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: likes_comment")

cursor.execute("DROP TABLE IF EXISTS likes_post")
sql = """CREATE TABLE likes_post (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255),
         likes_po_creation_data VARCHAR(255)
         )"""
cursor.execute(sql)

print("Created edge table: likes_post")

cursor.execute("DROP TABLE IF EXISTS studyat")
sql = """CREATE TABLE studyat (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255),
         sa_class_year INT
         )"""
cursor.execute(sql)

print("Created edge table: studyat")

cursor.execute("DROP TABLE IF EXISTS workat")
sql = """CREATE TABLE workat (
         EPOCH VARCHAR(255) NOT NULL,
         edge_label VARCHAR(255) NOT NULL,
         src VARCHAR(255),
         dst VARCHAR(255),
         wa_work_from INT
         )"""
cursor.execute(sql)

print("Created edge table: workat")

sql = f"SHOW TABLES FROM {args.db}"
cursor.execute(sql)
print("Execute SQL: " + sql)
results = cursor.fetchall()
for row in results:
    print(row[0])

db.close()

print(f"Done creating {len(results)} tables")