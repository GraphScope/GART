-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION gart" to load this file. \quit

CREATE FUNCTION gart_set_config(IN config_path TEXT)
RETURNS INTEGER
AS 'MODULE_PATHNAME', 'gart_set_config'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_get_connection(IN password TEXT)
RETURNS INTEGER
AS 'MODULE_PATHNAME', 'gart_get_connection'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_release_connection()
RETURNS INTEGER
AS 'MODULE_PATHNAME', 'gart_release_connection'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_define_graph(IN psql TEXT)
RETURNS INTEGER
AS 'MODULE_PATHNAME', 'gart_define_graph'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_define_graph_by_sql(IN psql_file TEXT)
RETURNS INTEGER
AS 'MODULE_PATHNAME', 'gart_define_graph_by_sql'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_define_graph_by_yaml(IN yaml_file TEXT)
RETURNS INTEGER
AS 'MODULE_PATHNAME', 'gart_define_graph_by_yaml'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_get_lastest_epoch()
RETURNS INTEGER
AS 'MODULE_PATHNAME', 'gart_get_lastest_epoch'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_launch_networkx_server(IN epoch INT, IN hostname TEXT, IN port INT)
RETURNS INTEGER
AS 'MODULE_PATHNAME', 'gart_launch_networkx_server'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_stop_networkx_server(IN server_handler INT)
RETURNS INTEGER
AS 'MODULE_PATHNAME', 'gart_stop_networkx_server'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_run_networkx_app(IN server_handler INT, IN script TEXT)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_run_networkx_app'
LANGUAGE C STRICT VOLATILE;

CREATE TYPE server_info_type AS (
    id INT,
    hostname TEXT,
    port INT,
    read_epoch INT
);

CREATE FUNCTION gart_show_networkx_server_info()
RETURNS SETOF server_info_type
AS 'MODULE_PATHNAME', 'gart_show_networkx_server_info'
LANGUAGE C STRICT VOLATILE;
