#!/bin/bash
#
# A script to perform tests for gart.

gart_engine_dir="../../../vegito/test"
gart_test_dir="../../../vegito/test"
gart_test_data_dir="../../../vegito/test/data"
gart_test_config_dir="../../../vegito/test/config"
analytical_engine_dir="../build"

socket_file="/opt/tmp/tmp.sock"

function start_vineyard() {
  sudo pkill vineyardd || true
  sudo pkill etcd || true
  echo "[INFO] vineyardd will using the socket_file on ${socket_file}"
  #start vineyardd
  touch ${socket_file}
  /usr/local/bin/vineyardd  --etcd_endpoint=127.0.0.1:2379 --socket ${socket_file} --size 500G &
  sleep 5
  info "vineyardd started."
}

function start_gart_writer() {
  #start gart writer
  pushd ${gart_engine_dir}
  mpirun -n 2 run_test.sh
  sleep 5
  info "gart writer started."
  popd
}


function start_gart_reader() {
  #start gart reader
  pushd ${analytical_engine_dir}
  mpirun -n 2 ./run_gart_reader
  popd
}

start_vineyard
start_gart_writer
echo "gart_reader..."
start_gart_reader
