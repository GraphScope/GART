#!/bin/bash

etcd_endpoint=$1
etcd_prefix=$2
read_epoch=$3

export READ_EPOCH=$read_epoch
export ETCD_SERVER=$etcd_endpoint
export ETCD_PREFIX=$etcd_prefix

APP_NAME="grin_executor"

# Start the GIE executor
cd /home/graphscope/GraphScope/interactive_engine/executor/assembly/grin_gart
# run backgorund and redirect stdout and stderr to /home/graphscope/gie_executor.log
./target/release/grin_executor ../../../assembly/src/conf/graphscope/log4rs.yml /home/graphscope/gie-executor-config.properties > /home/graphscope/gie_executor.log 2>&1 &