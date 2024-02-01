#! /usr/bin/bash

username=$1
password=$2

. /opt/ssj/projects/gart/apps/pgx/gart-pgx-config-template.sh

echo "username: $username"
echo "password: $password"
echo "GART_HOME: $GART_HOME"
echo "KAFKA_HOME: $KAFKA_HOME"

(export KAFKA_HOME=$KAFKA_HOME; cd $GART_HOME; \
./gart --db-type postgresql -u $username -p $password -r schema/rgmapping-ldbc.yaml --v6d-sock ldbc.sock -e 127.0.0.1:23760 --subgraph-num 4 --enable-bulkload 1 2>&1 \
 | tee /opt/postgresql/gart.log &)

# (export KAFKA_HOME=$KAFKA_HOME; cd $GART_HOME;./stop-gart --kill-v6d-sock ldbc.sock)
