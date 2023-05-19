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

    parser.add_argument("--data_dir",
                        help="LDBC dataset directory (dynamic)")

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

base_dir = args.data_dir
db = pymysql.connect(host=args.host,
                     port=int(args.port),
                     user=args.user,
                     password=args.password,
                     database=args.db)
cursor = db.cursor()

print("01. Inserting organisation table...")
file_name = base_dir + "/organisation_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, org_id, org_type, org_name, org_url = line.strip().split("|")
        org_name = org_name.replace("\'", "")
        org_url = org_url.replace("\'", "")
        cursor.execute(f"insert into organisation values('{epoch}', '{org_id}',"
                       f"'{org_type}', '{org_name}', '{org_url}')")
        line = f.readline()
        num_lines += 1
db.commit()
print(f"01. Insert {num_lines} rows into organisation table")

print("02. Inserting place table...")
file_name = base_dir + "/place_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, pla_id, pla_name, pla_url, pla_type = line.strip().split("|")
        pla_name = pla_name.replace("\'", "")
        pla_url = pla_url.replace("\'", "")
        cursor.execute(f"insert into place values('{epoch}', '{pla_id}',"
                       f"'{pla_name}', '{pla_url}', '{pla_type}')")
        line = f.readline()
        num_lines += 1
db.commit()
print(f"02. Insert {num_lines} rows into place table")

print("03. Inserting tag table...")
file_name = base_dir + "/tag_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, tag_id, tag_name, tag_url = line.strip().split("|")
        tag_url = tag_url.replace("\'", "")
        cursor.execute(f"insert into tag values('{epoch}', '{tag_id}',"
                       f"'{tag_name}', '{tag_url}')")
        line = f.readline()
        num_lines += 1
db.commit()
print(f"03. Insert {num_lines} rows into tag table")

print("04. Inserting tagclass table...")
file_name = base_dir + "/tagclass_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, tagc_id, tagc_name, tagc_url = line.strip().split("|")
        tagc_url = tagc_url.replace("\'", "")
        cursor.execute(f"insert into tagclass values('{epoch}', '{tagc_id}',"
                       f"'{tagc_name}', '{tagc_url}')")
        line = f.readline()
        num_lines += 1
db.commit()
print(f"04. Insert {num_lines} rows into tagclass table")

print("05. Inserting person table...")
file_name = base_dir + "/person_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, p_id, p_first_name, p_last_name, p_gender, p_birthday, p_creation_date, p_location_ip, p_browser_used = line.strip().split("|")
        cursor.execute(f"insert into person values('{epoch}', '{p_id}', "
                       f"'{p_first_name}', '{p_last_name}', '{p_gender}', "
                       f"'{p_birthday}', '{p_creation_date}', "
                       f"'{p_location_ip}', '{p_browser_used}')")
        line = f.readline()
        num_lines += 1
db.commit()
print(f"05. Insert {num_lines} rows into person table")

print("06. Inserting comment table...")
file_name = base_dir + "/comment_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, co_id, co_creation_date, co_location_ip, co_browser_used, co_content, co_length = line.strip().split("|")
        co_content = co_content.replace("\'", "")
        cursor.execute(f"insert into comment values('{epoch}', '{co_id}', "
                       f"'{co_creation_date}', '{co_location_ip}', "
                       f"'{co_browser_used}', '{co_content}', {co_length})")
        line = f.readline()
        num_lines += 1
db.commit()
print(f"06. Insert {num_lines} rows into comment table")

print("07. Inserting post table...")
file_name = base_dir + "/post_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, po_id, po_image_file, po_creation_date, po_location_ip, po_browser_used, po_language, po_content, po_length = line.strip().split("|")
        po_content = po_content.replace("\'", "")
        cursor.execute(f"insert into post values('{epoch}', '{po_id}', "
                       f"'{po_image_file}', '{po_creation_date}', "
                       f"'{po_location_ip}', '{po_browser_used}', "
                       f"'{po_language}', '{po_content}', {po_length})")
        line = f.readline()
        num_lines += 1
db.commit()
print(f"07. Insert {num_lines} rows into post table")

print("08. Inserting forum table...")
file_name = base_dir + "/forum_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, fo_id, fo_title, fo_creation_date = line.strip().split("|")
        fo_title = fo_title.replace("\'", "")
        cursor.execute(f"insert into forum values('{epoch}', '{fo_id}', "
                       f"'{fo_title}', '{fo_creation_date}')")
        line = f.readline()
        num_lines += 1
db.commit()
print(f"08. Insert {num_lines} rows into forum table")

# insert edge tables

print("09. Inserting org_islocationin table...")
file_name = base_dir + "/organisation_isLocatedIn_place_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute(f"insert into org_islocationin values('{epoch}', "
                       f"'{edge_label}', '{src}', '{dst}')")
        line = f.readline()
        num_lines += 1
db.commit()
print(f"09. Insert {num_lines} rows into org_islocationin table")

print("10. Inserting ispartof table...")
file_name = base_dir + "/place_isPartOf_place_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute(f"insert into ispartof values('{epoch}', "
                       f"'{edge_label}', '{src}', '{dst}')")
        line = f.readline()
        num_lines += 1
db.commit()
print(f"10. Insert {num_lines} rows into ispartof table")

print("11. Inserting issubclassof table...")
file_name = base_dir + "/tagclass_isSubclassOf_tagclass_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into issubclassof values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"11. Insert {num_lines} rows into issubclassof table")

print("12. Inserting hastype table...")
file_name = base_dir + "/tag_hasType_tagclass_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into hastype values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"12. Insert {num_lines} rows into hastype table")

print("13. Inserting comment_hascreator table...")
file_name = base_dir + "/comment_hasCreator_person_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into comment_hascreator values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"13. Insert {num_lines} rows into comment_hascreator table")

print("14. Inserting comment_hastag table...")
file_name = base_dir + "/comment_hasTag_tag_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into comment_hastag values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"14. Insert {num_lines} rows into comment_hastag table")

print("15. Inserting comment_islocationin table...")
file_name = base_dir + "/comment_isLocatedIn_place_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into comment_islocationin values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"15. Insert {num_lines} rows into comment_islocationin table")

print("16. Inserting comment_replyof table...")
file_name = base_dir + "/comment_replyOf_comment_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into replyof_comment values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"16. Insert {num_lines} rows into comment_replyof table")

print("17. Inserting replyof_post table...")
file_name = base_dir + "/comment_replyOf_post_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into replyof_post values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"17. Insert {num_lines} rows into replyof_post table")

print("18. Inserting post_hascreator table...")
file_name = base_dir + "/post_hasCreator_person_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into post_hascreator values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"18. Insert {num_lines} rows into post_hascreator table")

print("19. Inserting post_hasTag table...")
file_name = base_dir + "/post_hasTag_tag_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into post_hastag values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"19. Insert {num_lines} rows into post_hastag table")

print("20. Inserting post_islocatedin table...")
file_name = base_dir + "/post_isLocatedIn_place_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into post_islocationin values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"20. Insert {num_lines} rows into post_islocationin table")

print("21. Inserting forum_containerof table...")
file_name = base_dir + "/forum_containerOf_post_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into forum_containerof values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"21. Insert {num_lines} rows into forum_containerof table")

print("22. Inserting forum_hasmember table...")
file_name = base_dir + "/forum_hasModerator_person_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into forum_hasmoderator values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"22. Insert {num_lines} rows into forum_hasmoderator table")

print("23. Inserting forum_hastag table...")
file_name = base_dir + "/forum_hasTag_tag_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into forum_hastag values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"23. Insert {num_lines} rows into forum_hastag table")

print("24. Inserting person_hasinterest table...")
file_name = base_dir + "/person_hasInterest_tag_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into person_hasinterest values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"24. Insert {num_lines} rows into person_hasinterest table")

print("25. Inserting person_islocationin table...")
file_name = base_dir + "/person_isLocatedIn_place_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst = line.strip().split("|")
        cursor.execute("insert into person_islocationin values('%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"25. Insert {num_lines} rows into person_islocationin table")

print("26. Inserting forum_hasmember table...")
file_name = base_dir + "/forum_hasMember_person_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst, fo_hm_join_data = line.strip().split("|")
        cursor.execute("insert into forum_hasmember values('%s', '%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst, fo_hm_join_data))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"26. Insert {num_lines} rows into forum_hasmember table")

print("27. Inserting knows table...")
file_name = base_dir + "/person_knows_person_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst, kn_creation_data = line.strip().split("|")
        cursor.execute("insert into knows values('%s', '%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst, kn_creation_data))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"27. Insert {num_lines} rows into knows table")

print("28. Inserting likes_comment table...")
file_name = base_dir + "/person_likes_comment_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst, likes_co_creation_data = line.strip().split("|")
        cursor.execute("insert into likes_comment values('%s', '%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst, likes_co_creation_data))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"28. Insert {num_lines} rows into likes_comment table")

print("29. Inserting likes_post table...")
file_name = base_dir + "/person_likes_post_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst, likes_po_creation_data = line.strip().split("|")
        cursor.execute("insert into likes_post values('%s', '%s', '%s', '%s', '%s')" %
                    (epoch, edge_label, src, dst, likes_po_creation_data))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"29. Insert {num_lines} rows into likes_post table")

print("30. Inserting studyat table...")
file_name = base_dir + "/person_studyAt_organisation_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst, sa_class_year = line.strip().split("|")
        cursor.execute("insert into studyat values('%s', '%s', '%s', '%s', %s)" %
                    (epoch, edge_label, src, dst, sa_class_year))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"30. Insert {num_lines} rows into studyat table")

print("31. Inserting workat table...")
file_name = base_dir + "/person_workAt_organisation_0_0.csv"
with open(file_name, "r", encoding="UTF-8") as f:
    line = f.readline()
    num_lines = 0
    while line:
        _, epoch, edge_label, src, dst, wa_work_from = line.strip().split("|")
        cursor.execute("insert into workat values('%s', '%s', '%s', '%s', %s)" %
                    (epoch, edge_label, src, dst, wa_work_from))
        line = f.readline()
        num_lines += 1
db.commit()
print(f"31. Insert {num_lines} rows into workat table")

db.close()
