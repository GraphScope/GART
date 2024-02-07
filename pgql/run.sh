#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <sql2yaml|yaml2sql|sql2yaml_str> <input> <output>"
    exit 1
fi

type=$1    # sql2yaml or yaml2sql
input=$2
output=$3

if [ "$type" != "sql2yaml" ] && [ "$type" != "yaml2sql" ] && [ "$type" != "sql2yaml_str" ]; then
    echo "Invalid type: $type"
    exit 1
fi

export MAVEN_OPTS="-Xms512m -Xmx1024m -Xss16m $MAVEN_OPTS"

mvn clean package exec:java -Dexec.mainClass="gart.pgql.Main" -Dexec.cleanupDaemonThreads=false -Dexec.args="$type $input $output"
