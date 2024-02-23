-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION log" to load this file. \quit

CREATE FUNCTION pg_all_queries(OUT query TEXT, pid OUT TEXT)
RETURNS SETOF RECORD
AS 'MODULE_PATHNAME', 'pg_all_queries'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_set_config(IN config_path TEXT)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_set_config'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_get_connection(IN password TEXT)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_get_connection'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_release_connection()
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_release_connection'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_define_graph(IN psql TEXT)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_define_graph'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_define_graph_by_sql(IN psql_file TEXT)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_define_graph_by_sql'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_define_graph_by_yaml(IN yaml_file TEXT)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_define_graph_by_yaml'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_get_lastest_epoch()
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_get_lastest_epoch'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_launch_networkx_server(IN epoch INT, IN server_addr TEXT)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_launch_networkx_server'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_stop_networkx_server(IN server_handler INT)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_stop_networkx_server'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_run_networkx_app(IN server_handler INT, IN script TEXT)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_run_networkx_app'
LANGUAGE C STRICT VOLATILE;
