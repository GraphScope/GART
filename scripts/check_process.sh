#!/bin/bash

# Define the list of pids with process names

# $1 format: declare -A pids=([key2]="value2" [key1]="value1" )

v6d_sock=$1

eval "$2"

for process_name in ${!pids[@]}; do
    process_pid=${pids[$process_name]}
    if [[ -z $process_pid || ! $process_pid =~ ^[0-9]+$ ]]; then
        echo "Invalid pid $process_pid for process $process_name"
        unset pids[$process_name]
        continue
    fi
done

echo "Check Process with v6d_sock: $v6d_sock"

if [[ ${#pids[@]} -eq 0 ]]; then
    echo "No valid pids found"
    exit -1
fi

while true; do
    # Loop through the list of pids and process names
    for process_name in ${!pids[@]}; do
        process_pid=${pids[$process_name]}

        # echo "Checking $process_name with pid $process_pid"

        # Check if the process is running
        if ! ps -p "$process_pid" > /dev/null; then
            echo "$process_name is not running"
            ./stop-gart --kill-v6d-sock $v6d_sock --is-abnormal
        fi
    done

    # Wait for 10 seconds and repeat the process
    sleep 10
done
