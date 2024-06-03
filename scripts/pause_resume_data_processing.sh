#!/bin/bash

op=$1

check_env_vars() {
    if [[ -z $DEBEZIUM_SERVICE ]] || [[ -z $DEBEZIUM_CONNECTOR_NAME ]]; then
        echo "Debezium service URL or connector name not set."
        exit 1
    fi
}

if [[ $op == "pause" ]]; then
    echo "Pausing data processing"
    check_env_vars
    response=$(curl -s -o /dev/null -w "%{http_code}" -X PUT "http://${DEBEZIUM_SERVICE}/connectors/${DEBEZIUM_CONNECTOR_NAME}/pause")
    if [[ $response != 202 ]]; then
        echo "Failed to pause connector"
        exit 1
    else
        echo "Connector paused successfully"
        exit 0
    fi
elif [[ $op == "resume" ]]; then
    echo "Resuming data processing"
    check_env_vars
    response=$(curl -s -o /dev/null -w "%{http_code}" -X PUT "http://${DEBEZIUM_SERVICE}/connectors/${DEBEZIUM_CONNECTOR_NAME}/resume")
    if [[ $response != 202 ]]; then
        echo "Failed to resume connector"
        exit 1
    else
        echo "Connector resumed successfully"
        exit 0
    fi
else
    echo "Invalid operation $op"
    exit 1
fi
