#! /usr/bin/bash

username=$1
password=$2

config_file="/opt/ssj/projects/gart/apps/pgx/gart-pgx-config-template.ini"

parse_ini() {
    section=$1
    key=$2
    awk -F '=' -v section="$section" -v key="$key" '
        /^\['"$section"'\]$/ {found=1; next}
        /^\[/ && found {exit}
        found && $1 == key {print $2; exit}
        ' "$config_file" | tr -d ' '
}

KAFKA_HOME=$(parse_ini "path" "KAFKA_HOME" $config_file)
GART_HOME=$(parse_ini "path" "GART_HOME" $config_file)

echo "username: $username"
echo "password: $password"
echo "GART_HOME: $GART_HOME"
echo "KAFKA_HOME: $KAFKA_HOME"

(export KAFKA_HOME=$KAFKA_HOME; cd $GART_HOME/build; \
./gart --db-type postgresql -u $username -p $password -r schema/rgmapping-ldbc.yaml --v6d-sock ldbc.sock -e 127.0.0.1:23760 --subgraph-num 4 --enable-bulkload 1 2>&1 \
 | tee /opt/postgresql/gart.log &)

# (export KAFKA_HOME=$KAFKA_HOME; cd $GART_HOME/build;./stop-gart --kill-v6d-sock ldbc.sock)
