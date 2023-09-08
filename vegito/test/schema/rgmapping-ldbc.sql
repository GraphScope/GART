CREATE PROPERTY GRAPH ldbc
  VERTEX TABLES (
    "organisation"
      KEY ( "org_id" )
      PROPERTIES ( "org_id", "org_type", "org_name", "org_url" ),
    "place"
      KEY ( "pla_id" )
      PROPERTIES ( "pla_id", "pla_name", "pla_url", "pla_type" ),
    "tag"
      KEY ( "tag_id" )
      PROPERTIES ( "tag_id", "tag_name", "tag_url" ),
    "tagclass"
      KEY ( "tagc_id" )
      PROPERTIES ( "tagc_id", "tagc_name", "tagc_url" ),
    "person"
      KEY ( "p_id" )
      PROPERTIES ( "p_id", "p_first_name", "p_last_name", "p_gender", "p_birthday", "p_creation_date", "p_location_ip", "p_browser_used" ),
    "comment"
      KEY ( "co_id" )
      PROPERTIES ( "co_id", "co_creation_date", "co_location_ip", "co_browser_used", "co_content", "co_length" ),
    "post"
      KEY ( "po_id" )
      PROPERTIES ( "po_id", "po_image_file", "po_creation_date", "po_location_ip", "po_browser_used", "po_language", "po_content", "po_length" ),
    "forum"
      KEY ( "fo_id" )
      PROPERTIES ( "fo_id", "fo_title", "fo_creation_date" )
  )
  EDGE TABLES (
    "org_islocationin"
      SOURCE KEY ( "src" ) REFERENCES "organisation"
      DESTINATION KEY ( "dst" ) REFERENCES "place"
      NO PROPERTIES,
    "ispartof"
      SOURCE KEY ( "src" ) REFERENCES "place"
      DESTINATION KEY ( "dst" ) REFERENCES "place"
      NO PROPERTIES,
    "issubclassof"
      SOURCE KEY ( "src" ) REFERENCES "tagclass"
      DESTINATION KEY ( "dst" ) REFERENCES "tagclass"
      NO PROPERTIES,
    "hastype"
      SOURCE KEY ( "src" ) REFERENCES "tag"
      DESTINATION KEY ( "dst" ) REFERENCES "tagclass"
      NO PROPERTIES,
    "comment_hascreator"
      SOURCE KEY ( "src" ) REFERENCES "comment"
      DESTINATION KEY ( "dst" ) REFERENCES "person"
      NO PROPERTIES,
    "comment_hastag"
      SOURCE KEY ( "src" ) REFERENCES "comment"
      DESTINATION KEY ( "dst" ) REFERENCES "tag"
      NO PROPERTIES,
    "comment_islocationin"
      SOURCE KEY ( "src" ) REFERENCES "comment"
      DESTINATION KEY ( "dst" ) REFERENCES "place"
      NO PROPERTIES,
    "replyof_comment"
      SOURCE KEY ( "src" ) REFERENCES "comment"
      DESTINATION KEY ( "dst" ) REFERENCES "comment"
      NO PROPERTIES,
    "replyof_post"
      SOURCE KEY ( "src" ) REFERENCES "comment"
      DESTINATION KEY ( "dst" ) REFERENCES "post"
      NO PROPERTIES,
    "post_hascreator"
      SOURCE KEY ( "src" ) REFERENCES "post"
      DESTINATION KEY ( "dst" ) REFERENCES "person"
      NO PROPERTIES,
    "post_hastag"
      SOURCE KEY ( "src" ) REFERENCES "post"
      DESTINATION KEY ( "dst" ) REFERENCES "tag"
      NO PROPERTIES,
    "post_islocationin"
      SOURCE KEY ( "src" ) REFERENCES "post"
      DESTINATION KEY ( "dst" ) REFERENCES "place"
      NO PROPERTIES,
    "forum_containerof"
      SOURCE KEY ( "src" ) REFERENCES "forum"
      DESTINATION KEY ( "dst" ) REFERENCES "post"
      NO PROPERTIES,
    "forum_hasmoderator"
      SOURCE KEY ( "src" ) REFERENCES "forum"
      DESTINATION KEY ( "dst" ) REFERENCES "person"
      NO PROPERTIES,
    "forum_hastag"
      SOURCE KEY ( "src" ) REFERENCES "forum"
      DESTINATION KEY ( "dst" ) REFERENCES "tag"
      NO PROPERTIES,
    "person_hasinterest"
      SOURCE KEY ( "src" ) REFERENCES "person"
      DESTINATION KEY ( "dst" ) REFERENCES "tag"
      NO PROPERTIES,
    "person_islocationin"
      SOURCE KEY ( "src" ) REFERENCES "person"
      DESTINATION KEY ( "dst" ) REFERENCES "place"
      NO PROPERTIES,
    "forum_hasmember"
      SOURCE KEY ( "src" ) REFERENCES "forum"
      DESTINATION KEY ( "dst" ) REFERENCES "person"
      PROPERTIES ( "fo_hm_join_date" ),
    "knows"
      SOURCE KEY ( "src" ) REFERENCES "person"
      DESTINATION KEY ( "dst" ) REFERENCES "person"
      PROPERTIES ( "kn_creation_date" ),
    "likes_comment"
      SOURCE KEY ( "src" ) REFERENCES "person"
      DESTINATION KEY ( "dst" ) REFERENCES "comment"
      PROPERTIES ( "likes_co_creation_date" ),
    "likes_post"
      SOURCE KEY ( "src" ) REFERENCES "person"
      DESTINATION KEY ( "dst" ) REFERENCES "post"
      PROPERTIES ( "likes_po_creation_date" ),
    "studyat"
      SOURCE KEY ( "src" ) REFERENCES "person"
      DESTINATION KEY ( "dst" ) REFERENCES "organisation"
      PROPERTIES ( "sa_class_year" ),
    "workat"
      SOURCE KEY ( "src" ) REFERENCES "person"
      DESTINATION KEY ( "dst" ) REFERENCES "organisation"
      PROPERTIES ( "wa_work_from" )
  )