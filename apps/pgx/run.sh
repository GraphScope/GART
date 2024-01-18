#! /usr/bin/bash

(export KAFKA_HOME=/opt/wanglei/kafka_2.13-3.4.0; cd /opt/ssj/projects/gart/build; ./gart --db-type postgresql -u dbuser -p123456 -r schema/rgmapping-ldbc.yaml --v6d-sock ldbc.sock -e 127.0.0.1:23760 --subgraph-num 4 --enable-bulkload 1 2>&1 | tee /opt/postgresql/gart.log &)
