#!/bin/bash

sudo apt update

# glogs
git clone https://github.com/google/glog.git
cd glog
cmake -S . -B build -G "Unix Makefiles"
sudo cmake --build build --target install

# gflags
sudo apt-get install libgflags-dev

# etcd
sudo apt-get install libboost-all-dev libssl-dev
sudo apt-get install libgrpc-dev \
        libgrpc++-dev \
        libprotobuf-dev \
        protobuf-compiler-grpc
git clone https://github.com/microsoft/cpprestsdk.git
cd cpprestsdk
mkdir build && cd build
cmake .. -DCPPREST_EXCLUDE_WEBSOCKETS=ON
make -j12 && make install
git clone https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3.git
cd etcd-cpp-apiv3
mkdir build && cd build
cmake ..
make -j$(nproc) && make install

# TBB
git clone https://github.com/oneapi-src/oneTBB.git
cd oneTBB
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j12 && sudo make install

# librdkafka
sudo apt install librdkafka-dev

# vineyard
pip3 install vineyard

# Kafka
wget https://dlcdn.apache.org/kafka/3.4.0/kafka_2.13-3.4.0.tgz
tar -xzf kafka_2.13-3.4.0.tgz
rm kafka_2.13-3.4.0.tgz
export KAFKA_HOME=`pwd`/kafka_2.13-3.4.0
echo "delete.topic.enable=true" >> $KAFKA_HOME/config/server.properties

# Maxwell
wget https://github.com/zendesk/maxwell/releases/download/v1.40.0/maxwell-1.40.0.tar.gz
tar zxvf maxwell-1.40.0.tar.gz
rm maxwell-1.40.0.tar.gz
export MAXWELL_HOME=`pwd`/maxwell-1.40.0