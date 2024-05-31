#!/bin/bash

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <sql2yaml|yaml2sql|sqlStr2yaml|yaml-check|yaml-format> <input> <output>"
    exit 1
fi

type=$1    # sql2yaml or yaml2sql
input=$2
output=$3

case "$type" in
    "sql2yaml"|"yaml2sql"|"sqlStr2yaml"|"yaml-check"|"yaml-format")
        # If $type is one of the expected values, do nothing
        ;;
    *)
        # If $type is not a recognized type, output an error message and exit
        echo "Invalid type: $type"
        exit 1
        ;;
esac

export MAVEN_OPTS="-Xms512m -Xmx1024m -Xss16m $MAVEN_OPTS"

mvn clean package exec:java -Dexec.mainClass="gart.pgql.Main" -Dexec.cleanupDaemonThreads=false -Dexec.args="$type \"$input\" $output"
