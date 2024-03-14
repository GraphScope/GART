LOAD 'age';
LOAD '$libdir/plugins/age.so';
SET search_path = ag_catalog, "$user", public;

SELECT * FROM ag_catalog.create_graph('ldbc');

SELECT create_vlabel('ldbc', 'Organisation');
SELECT create_vlabel('ldbc', 'Place');
SELECT create_vlabel('ldbc', 'Tag');
SELECT create_vlabel('ldbc', 'TagClass');
SELECT create_vlabel('ldbc', 'Person');
SELECT create_vlabel('ldbc', 'Comment');
SELECT create_vlabel('ldbc', 'Post');
SELECT create_vlabel('ldbc', 'Forum');

SELECT create_elabel('ldbc','org_islocationin');
SELECT create_elabel('ldbc','ispartof');
SELECT create_elabel('ldbc','issubclassof');
SELECT create_elabel('ldbc','hastype');
SELECT create_elabel('ldbc','comment_hascreator');
SELECT create_elabel('ldbc','comment_hastag');
SELECT create_elabel('ldbc','comment_islocationin');
SELECT create_elabel('ldbc','replyof_comment');
SELECT create_elabel('ldbc','replyof_post');
SELECT create_elabel('ldbc','post_hascreator');
SELECT create_elabel('ldbc','post_hastag');
SELECT create_elabel('ldbc','post_islocationin');
SELECT create_elabel('ldbc','forum_containerof');
SELECT create_elabel('ldbc','forum_hasmoderator');
SELECT create_elabel('ldbc','forum_hastag');
SELECT create_elabel('ldbc','person_hasinterest');
SELECT create_elabel('ldbc','person_islocationin');

SELECT create_elabel('ldbc','forum_hasmember');
SELECT create_elabel('ldbc','knows');
SELECT create_elabel('ldbc','likes_comment');
SELECT create_elabel('ldbc','likes_post');
SELECT create_elabel('ldbc','studyat');
SELECT create_elabel('ldbc','workat');

SELECT load_labels_from_file('ldbc',
                             'Organisation',
                             '/opt/ssj/dataset/ldbc-0.1-age/organisation_0_0.csv');

SELECT load_labels_from_file('ldbc',
                             'Place',
                             '/opt/ssj/dataset/ldbc-0.1-age/place_0_0.csv');

SELECT load_labels_from_file('ldbc',
                             'Tag',
                             '/opt/ssj/dataset/ldbc-0.1-age/tag_0_0.csv');

SELECT load_labels_from_file('ldbc',
                             'TagClass',
                             '/opt/ssj/dataset/ldbc-0.1-age/tagclass_0_0.csv');

SELECT load_labels_from_file('ldbc',
                             'Person',
                             '/opt/ssj/dataset/ldbc-0.1-age/person_0_0.csv');

SELECT load_labels_from_file('ldbc',
                             'Comment',
                             '/opt/ssj/dataset/ldbc-0.1-age/comment_0_0.csv');

SELECT load_labels_from_file('ldbc',
                             'Post',
                             '/opt/ssj/dataset/ldbc-0.1-age/post_0_0.csv');

SELECT load_labels_from_file('ldbc',
                             'Forum',
                             '/opt/ssj/dataset/ldbc-0.1-age/forum_0_0.csv');

SELECT load_edges_from_file('ldbc', 'org_islocationin',
     '/opt/ssj/dataset/ldbc-0.1-age/organisation_isLocatedIn_place_0_0.csv');

SELECT load_edges_from_file('ldbc', 'ispartof',
     '/opt/ssj/dataset/ldbc-0.1-age/place_isPartOf_place_0_0.csv');

SELECT load_edges_from_file('ldbc', 'issubclassof',
     '/opt/ssj/dataset/ldbc-0.1-age/tagclass_isSubclassOf_tagclass_0_0.csv');

SELECT load_edges_from_file('ldbc', 'hastype',
     '/opt/ssj/dataset/ldbc-0.1-age/tag_hasType_tagclass_0_0.csv');

SELECT load_edges_from_file('ldbc', 'hastype',
     '/opt/ssj/dataset/ldbc-0.1-age/tag_hasType_tagclass_0_0.csv');

SELECT load_edges_from_file('ldbc', 'comment_hascreator',
     '/opt/ssj/dataset/ldbc-0.1-age/comment_hasCreator_person_0_0.csv');

SELECT load_edges_from_file('ldbc', 'comment_hastag',
     '/opt/ssj/dataset/ldbc-0.1-age/comment_hasTag_tag_0_0.csv');

SELECT load_edges_from_file('ldbc', 'comment_islocationin',
     '/opt/ssj/dataset/ldbc-0.1-age/comment_isLocatedIn_place_0_0.csv');

SELECT load_edges_from_file('ldbc', 'replyof_comment',
     '/opt/ssj/dataset/ldbc-0.1-age/comment_replyOf_comment_0_0.csv');

SELECT load_edges_from_file('ldbc', 'replyof_post',
     '/opt/ssj/dataset/ldbc-0.1-age/comment_replyOf_post_0_0.csv');

SELECT load_edges_from_file('ldbc', 'post_hascreator',
     '/opt/ssj/dataset/ldbc-0.1-age/post_hasCreator_person_0_0.csv');

SELECT load_edges_from_file('ldbc', 'post_hastag',
     '/opt/ssj/dataset/ldbc-0.1-age/post_hasTag_tag_0_0.csv');

SELECT load_edges_from_file('ldbc', 'post_islocationin',
     '/opt/ssj/dataset/ldbc-0.1-age/post_isLocatedIn_place_0_0.csv');

SELECT load_edges_from_file('ldbc', 'forum_containerof',
     '/opt/ssj/dataset/ldbc-0.1-age/forum_containerOf_post_0_0.csv');

SELECT load_edges_from_file('ldbc', 'forum_hasmoderator',
     '/opt/ssj/dataset/ldbc-0.1-age/forum_hasModerator_person_0_0.csv');

SELECT load_edges_from_file('ldbc', 'forum_hastag',
     '/opt/ssj/dataset/ldbc-0.1-age/forum_hasTag_tag_0_0.csv');

SELECT load_edges_from_file('ldbc', 'person_hasinterest',
     '/opt/ssj/dataset/ldbc-0.1-age/person_hasInterest_tag_0_0.csv');

SELECT load_edges_from_file('ldbc', 'person_islocationin',
     '/opt/ssj/dataset/ldbc-0.1-age/person_isLocatedIn_place_0_0.csv');

SELECT load_edges_from_file('ldbc', 'forum_hasmember',
     '/opt/ssj/dataset/ldbc-0.1-age/forum_hasMember_person_0_0.csv');

SELECT load_edges_from_file('ldbc', 'knows',
     '/opt/ssj/dataset/ldbc-0.1-age/person_knows_person_0_0.csv');

SELECT load_edges_from_file('ldbc', 'likes_comment',
     '/opt/ssj/dataset/ldbc-0.1-age/person_likes_comment_0_0.csv');

SELECT load_edges_from_file('ldbc', 'likes_post',
     '/opt/ssj/dataset/ldbc-0.1-age/person_likes_post_0_0.csv');

SELECT load_edges_from_file('ldbc', 'studyat',
     '/opt/ssj/dataset/ldbc-0.1-age/person_studyAt_organisation_0_0.csv');

SELECT load_edges_from_file('ldbc', 'workat',
     '/opt/ssj/dataset/ldbc-0.1-age/person_workAt_organisation_0_0.csv');
