-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION log" to load this file. \quit

CREATE FUNCTION pg_all_queries(OUT query TEXT, pid OUT TEXT)
RETURNS SETOF RECORD
AS 'MODULE_PATHNAME', 'pg_all_queries'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION gart_get_connection(IN config_path TEXT, IN password TEXT)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'gart_get_connection'
LANGUAGE C STRICT VOLATILE;