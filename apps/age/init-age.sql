LOAD 'age';
SET search_path = ag_catalog, "$user", public;

SELECT * FROM ag_catalog.create_graph('ldbc');
SELECT create_vlabel('ldbc', 'Person');
SELECT create_elabel('ldbc','knows');
SELECT load_labels_from_file('ldbc',
                             'Person',
                             '/opt/ssj/dataset/ldbc-0.1-age/person.csv');
SELECT load_edges_from_file('ldbc', 'knows',
     '/opt/ssj/dataset/ldbc-0.1-age/knows.csv');
