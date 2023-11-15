#! /usr/bin/env python3

import readline
import time

from gremlin_python.driver.client import Client

client = Client(f"ws://127.0.0.1:8182/gremlin", "g")

# if client is effectively connected
try:
    client.submit("g.V().limit(1)")
except:
    print("Connection failed!")
    exit(-1)

while 1:
    query = input("gremlin> ")
    if query == "quit" or query == "q" or query == "exit":
        print("Bye!")
        break
    elif len(query) > 0:
        try:
            start_time = time.time()
            callback = client.submit(query)
            print(callback.all().result())
            end_time = time.time()
            execution_time = end_time - start_time
            print(f"Execution time: { int(execution_time * 1000) } ms")
        except:
            print("Something went wrong!")
            continue
        else:
            pass


# g.V().as('a').hasLabel('PERSON').out('2..3', 'knows').with('PATH_OPT', 'SIMPLE').out().where(eq('a'))

# g.V().hasLabel('person').valueMap().limit(1)

# LDBC BI-3
# g.V().has("PLACE", "name", "China").in("ISPARTOF").in("PERSON_ISLOCATEDIN").as("person").in("HASMODERATOR").as("forum") .out("CONTAINEROF").in("0..5", "REPLYOF_COMMENT", "REPLYOF_POST").endV().as("msg").out("POST_HASTAG","FORUM_HASTAG").out("HASTYPE").has("TAGCLASS", "name", "Song").select("msg").group().by(select("forum").as("forum")).by(dedup().count().as("msgCnt")).order().by(select("msgCnt"), desc).by(select("forum").by("id"), asc).limit(20).select("forum", "msgCnt").by(valueMap("id", "title", "creationDate")).by()

# LDBC BI-5
# g.V().has("TAG", "name", "Sikh_Empire").in("COMMENT_HASTAG","POST_HASTAG","FORUM_HASTAG").as("msg") .out("COMMENT_HASCREATOR","POST_HASCREATOR").as("person").union( select("msg").as("msg1"), select("msg").in("LIKES_COMMENT","LIKES_POST").as("like"), select("msg").in("REPLYOF_COMMENT", "REPLYOF_POST").as("reply")) .group() .by(select("person")) .by( select("msg1").count().as("msgCnt"), select("like").count().as("likeCnt"), select("reply").count().as("replyCnt")) .select(keys).as("person") .select(expr("1*@msgCnt+2*@replyCnt+10*@likeCnt")).as("score") .order() .by(select("score"), desc) .by(select("person").by("id"), asc) .limit(100) .select("person", "replyCnt", "likeCnt", "msgCnt", "score") .by("id").by().by().by().by()

# LDBC BI-6
# g.V().has("TAG", "name", "Bob_Geldof").in("POST_HASTAG","FORUM_HASTAG","COMMENT_HASTAG").as("msg").out("COMMENT_HASCREATOR","POST_HASCREATOR").as("person").select("msg").in("LIKES_COMMENT","LIKES_POST").in("COMMENT_HASCREATOR","POST_HASCREATOR").inE("LIKES_COMMENT","LIKES_POST").group().by(select("person")).by(dedup().count()).order().by(select(values), desc).by(select(keys).values("id"), asc).limit(100)

# LDBC BI-14
# g.V().has("PLACE", "name", "India").in("ISPARTOF").as("city1").in("PERSON_ISLOCATEDIN").as("p1").both("KNOWS").as("p2").out("PERSON_ISLOCATEDIN").out("ISPARTOF").has("name", "Chile").select("p1").by(__.in("COMMENT_HASCREATOR","POST_HASCREATOR").hasLabel("COMMENT").out("REPLYOF_COMMENT","REPLYOF_POST").out("POST_HASCREATOR","COMMENT_HASCREATOR").where(P.eq("p2")).select("p1").dedup().count()).as("case1").select("p1").by(__.in("COMMENT_HASCREATOR","POST_HASCREATOR").in("REPLYOF_COMMENT","REPLYOF_POST").hasLabel("COMMENT").out("POST_HASCREATOR","COMMENT_HASCREATOR").where(P.eq("p2")).select("p1").dedup().count()).as("case2").select("p1").by(out("LIKES_COMMENT","LIKES_POST").hasLabel("POST", "COMMENT").out("POST_HASCREATOR","COMMENT_HASCREATOR").where(P.eq("p2")).select("p1").dedup().count()).as("case3").select("p1").by(__.in("POST_HASCREATOR","COMMENT_HASCREATOR").hasLabel("POST", "COMMENT").in("LIKES_COMMENT","LIKES_POST").where(P.eq("p2")).select("p1").dedup().count()).as("case4").select(expr("@case1 * 4 + @case2 * 1 + @case3 * 10 + @case4 * 1")).as("score").order().by(select("score"), desc).by(select("p1").by("id"), asc).by(select("p2").by("id"), asc).dedup("city1").select("p1", "p2", "score").by("id").by("id").by()
