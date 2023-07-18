#!/bin/bash

sudo apt update

# glogs
git clone https://github.com/google/glog.git
cd glog
cmake -S . -B build -G "Unix Makefiles"
sudo cmake --build build --target install
cd ..

# gflags
sudo apt-get install -y libgflags-dev

# etcd
sudo apt-get install -y libboost-all-dev libssl-dev
sudo apt-get install -y libgrpc-dev \
        libgrpc++-dev \
        libprotobuf-dev \
        protobuf-compiler-grpc
git clone https://github.com/microsoft/cpprestsdk.git
cd cpprestsdk
mkdir build && cd build
cmake .. -DCPPREST_EXCLUDE_WEBSOCKETS=ON
make -j && make install
cd ../..

git clone https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3.git
cd etcd-cpp-apiv3
mkdir build && cd build
cmake ..
make -j && make install
cd ../..

# TBB
git clone https://github.com/oneapi-src/oneTBB.git
cd oneTBB
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DTBB_TEST=OFF
make -j && sudo make install
cd ../..

# yaml-cpp
git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp
mkdir build && cd build
cmake -D BUILD_SHARED_LIBS=ON ..
make -j && sudo make install
cd ../..

pip3 install pyyaml

# librdkafka
sudo apt-get install -y librdkafka-dev

# vineyard
# pip3 install vineyard
sudo apt-get install -y ca-certificates \
                   doxygen \
                   libboost-all-dev \
                   libcurl4-openssl-dev \
                   libgrpc-dev \
                   libgrpc++-dev \
                   libmpich-dev \
                   libprotobuf-dev \
                   libssl-dev \
                   libunwind-dev \
                   libz-dev \
                   protobuf-compiler-grpc
wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb -O /tmp/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
sudo apt-get install -y -V /tmp/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
sudo apt update -y
sudo apt-get install -y libarrow-dev

pip3 install libclang

git clone https://github.com/v6d-io/v6d
cd v6d
git submodule update --init
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_VINEYARD_TESTS=OFF -DBUILD_VINEYARD_BENCHMARKS=OFF
make -j && sudo make install
cd ../..

# Kafka
wget https://archive.apache.org/dist/kafka/3.4.0/kafka_2.13-3.4.0.tgz
tar -xzf kafka_2.13-3.4.0.tgz
rm kafka_2.13-3.4.0.tgz
mv kafka_2.13-3.4.0 kafka
export KAFKA_HOME=`pwd`/kafka
echo "delete.topic.enable=true" >> $KAFKA_HOME/config/server.properties

# Maxwell
wget https://github.com/zendesk/maxwell/releases/download/v1.40.0/maxwell-1.40.0.tar.gz
tar zxvf maxwell-1.40.0.tar.gz
rm maxwell-1.40.0.tar.gz
mv maxwell-1.40.0 maxwell
export MAXWELL_HOME=`pwd`/maxwell

# Debezium
# we need to mkdir connect plugins for kafka
KAFKA_CONNECT_DIR=$KAFKA_HOME/connect
mkdir $KAFKA_CONNECT_DIR
# mysql connector
wget https://repo1.maven.org/maven2/io/debezium/debezium-connector-mysql/2.3.0.Final/debezium-connector-mysql-2.3.0.Final-plugin.tar.gz
tar zxvf debezium-connector-mysql-2.3.0.Final-plugin.tar.gz
rm debezium-connector-mysql-2.3.0.Final-plugin.tar.gz
mv debezium-connector-mysql/* $KAFKA_CONNECT_DIR
#postgresql connector
wget https://repo1.maven.org/maven2/io/debezium/debezium-connector-postgres/2.3.0.Final/debezium-connector-postgres-2.3.0.Final-plugin.tar.gz
tar zxvf debezium-connector-postgres-2.3.0.Final-plugin.tar.gz
rm debezium-connector-postgres-2.3.0.Final-plugin.tar.gz
mv debezium-connector-postgres/* $KAFKA_CONNECT_DIR
#link kafka connect plugins
ln -s $KAFKA_CONNECT_DIR/*.jar $KAFKA_HOME/libs/
#write config for kafka connect
echo "plugin.path=$KAFKA_CONNECT_DIR" >> $KAFKA_HOME/config/connect-standalone.properties

#write connect-debezium-mysql.properties
cat << EOT >> $KAFKA_HOME/config/connect-debezium-mysql.properties

name=test-connector
connector.class=io.debezium.connector.mysql.MySqlConnector
database.hostname=<mysql host>
database.port=<mysql port>
database.user=<mysql user>
database.password=<mysql password>
database.server.id=1
database.include.list=<which databse is needed to capture, e.g., ldbc>
database.history.kafka.bootstrap.servers=<kafka bootstrap servers, e.g., localhost:9092>
schema.history.internal.kafka.bootstrap.servers=<kafka bootstrap servers, e.g., localhost:9092>
include.schema.changes=true
tombstones.on.delete=false
topic.prefix=fullfillment
schema.history.internal.kafka.topic=schemahistory.fullfillment
transforms=Combine,ReplaceField
transforms.Combine.type=io.debezium.transforms.ByLogicalTableRouter
transforms.Combine.topic.regex=(.*)
transforms.Combine.topic.replacement=binlog
transforms.ReplaceField.type=org.apache.kafka.connect.transforms.ReplaceField$Value
transforms.ReplaceField.exclude=ts_ms,transaction
key.converter=org.apache.kafka.connect.json.JsonConverter
value.converter=org.apache.kafka.connect.json.JsonConverter
key.converter.schemas.enable=false
value.converter.schemas.enable=false

EOT

#write connect-debezium-postgresql.properties
cat << EOT >> $KAFKA_HOME/config/connect-debezium-postgresql.properties

name=test-connector
connector.class=io.debezium.connector.postgresql.PostgresConnector
database.hostname=<postgresql host>
database.port=<postgresql port>
database.user=<postgresql user>
database.password=<postgresql password>
database.dbname=<which databse is needed to capture, e.g., ldbc>
database.history.kafka.bootstrap.servers=<kafka bootstrap servers, e.g., localhost:9092>
schema.history.internal.kafka.bootstrap.servers=<kafka bootstrap servers, e.g., localhost:9092>
plugin.name=pgoutput
snapshot.mode=<if enable buldload, set as `always`, otherwise set as `never`>
database.server.id=1
include.schema.changes=true
tombstones.on.delete=false
publication.autocreate.mode=filtered
topic.prefix=fullfillment
schema.history.internal.kafka.topic=schemahistory.fullfillment
transforms=Combine,ReplaceField
transforms.Combine.type=io.debezium.transforms.ByLogicalTableRouter
transforms.Combine.topic.regex=(.*)
transforms.Combine.topic.replacement=binlog
transforms.ReplaceField.type=org.apache.kafka.connect.transforms.ReplaceField$Value
transforms.ReplaceField.exclude=ts_ms,transaction
key.converter=org.apache.kafka.connect.json.JsonConverter
value.converter=org.apache.kafka.connect.json.JsonConverter
key.converter.schemas.enable=false
value.converter.schemas.enable=false

EOT


# Install sqlalchemy, pymysql, psycopg2
pip3 install sqlalchemy pymysql psycopg2
