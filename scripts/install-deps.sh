#!/bin/bash

sudo apt update

sudo apt-get install -y cmake 
sudo apt-get install -y etcd
sudo apt-get install -y default-jdk

# gflags and glog
sudo apt-get install -y libgflags-dev libgoogle-glog-dev

# etcd-client
sudo apt-get install -y libboost-all-dev libssl-dev
sudo apt-get install -y libgrpc-dev \
        libgrpc++-dev \
        libprotobuf-dev \
        protobuf-compiler-grpc
git clone https://github.com/microsoft/cpprestsdk.git
cd cpprestsdk
mkdir -p build && cd build
cmake .. -DCPPREST_EXCLUDE_WEBSOCKETS=ON -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF
make -j && sudo make install
cd ../..

git clone https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3.git
cd etcd-cpp-apiv3
mkdir -p build && cd build
cmake ..
make -j && sudo make install
cd ../..

# TBB
git clone https://github.com/oneapi-src/oneTBB.git
cd oneTBB
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DTBB_TEST=OFF
make -j && sudo make install
cd ../..

# yaml-cpp
git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp
mkdir -p build && cd build
cmake -D BUILD_SHARED_LIBS=ON ..
make -j && sudo make install
cd ../..

pip3 install pyyaml

# librdkafka
sudo apt-get install -y librdkafka-dev

# Install sqlalchemy, pymysql, psycopg2
# for psycopg2, you need to install libpq-dev
sudo apt-get install -y libpq-dev 
pip3 install sqlalchemy pymysql psycopg2

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
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_VINEYARD_TESTS=OFF -DBUILD_VINEYARD_BENCHMARKS=OFF -DBUILD_SHARED_LIBS=ON -DBUILD_VINEYARD_LLM_CACHE=OFF
make -j && sudo make install
cd ../..

# libgrape-lite
git clone https://github.com/alibaba/libgrape-lite.git
cd libgrape-lite
mkdir -p build && cd build
cmake .. -DBUILD_LIBGRAPELITE_TESTS=OFF
make -j && sudo make install
cd ../..

# pgql-lang
git clone https://github.com/oracle/pgql-lang.git
(cd pgql-lang; sh install.sh)

# rapidjson
sudo apt-get install -y rapidjson-dev

# required python modules
pip3 install etcd3 msgpack grpcio grpcio-tools networkx

# Kafka
KAFKA_VERSION=3.7.0
KAFKA_SACALE_VERSION=2.13
KAFKA_PKG_NAME=kafka_$KAFKA_SACALE_VERSION-$KAFKA_VERSION
wget https://dlcdn.apache.org/kafka/$KAFKA_VERSION/$KAFKA_PKG_NAME.tgz
tar -xzf $KAFKA_PKG_NAME.tgz
rm $KAFKA_PKG_NAME.tgz
export KAFKA_HOME=`pwd`/$KAFKA_PKG_NAME
export KAFKA_CONFIG=$KAFKA_HOME/config
export KAFKA_PLUGIN=$KAFKA_HOME/connect
export KAFKA_LOGS=$KAFKA_HOME/logs
mkdir -p $KAFKA_PLUGIN  # we need to mkdir connect plugins for Kafka
echo "delete.topic.enable=true" >> $KAFKA_CONFIG/server.properties
#write config for kafka connect
echo "plugin.path=$KAFKA_PLUGIN" >> $KAFKA_CONFIG/connect-standalone.properties
mkdir -p $KAFKA_LOGS
sudo chmod -R 774 $KAFKA_LOGS # we need to change the permission of logs for PostgreSQL Extension

# Maxwell
wget https://github.com/zendesk/maxwell/releases/download/v1.40.0/maxwell-1.40.0.tar.gz
tar zxvf maxwell-1.40.0.tar.gz
rm maxwell-1.40.0.tar.gz
mv maxwell-1.40.0 maxwell
export MAXWELL_HOME=`pwd`/maxwell

# Debezium
DEBEZIUM_VERSION=2.5.2.Final
# mysql connector
wget https://repo1.maven.org/maven2/io/debezium/debezium-connector-mysql/$DEBEZIUM_VERSION/debezium-connector-mysql-$DEBEZIUM_VERSION-plugin.tar.gz
tar zxvf debezium-connector-mysql-$DEBEZIUM_VERSION-plugin.tar.gz
rm debezium-connector-mysql-$DEBEZIUM_VERSION-plugin.tar.gz
mv debezium-connector-mysql/* $KAFKA_PLUGIN
rm -rf debezium-connector-mysql
#postgresql connector
wget https://repo1.maven.org/maven2/io/debezium/debezium-connector-postgres/$DEBEZIUM_VERSION/debezium-connector-postgres-$DEBEZIUM_VERSION-plugin.tar.gz
tar zxvf debezium-connector-postgres-$DEBEZIUM_VERSION-plugin.tar.gz
rm debezium-connector-postgres-$DEBEZIUM_VERSION-plugin.tar.gz
mv debezium-connector-postgres/* $KAFKA_PLUGIN
rm -rf debezium-connector-postgres
#link kafka connect plugins
ln -s $KAFKA_PLUGIN/*.jar $KAFKA_HOME/libs/

COMM_CONFIG=$(cat <<EOT
database.server.id=1
include.schema.changes=false
tombstones.on.delete=false
topic.prefix=fullfillment
schema.history.internal.kafka.topic=schemahistory.fullfillment
transforms=Combine,ReplaceField
transforms.Combine.type=io.debezium.transforms.ByLogicalTableRouter
transforms.Combine.topic.regex=(.*)
transforms.Combine.topic.replacement=binlog
transforms.ReplaceField.type=org.apache.kafka.connect.transforms.ReplaceField\$Value
transforms.ReplaceField.exclude=ts_ms,transaction
key.converter=org.apache.kafka.connect.json.JsonConverter
value.converter=org.apache.kafka.connect.json.JsonConverter
key.converter.schemas.enable=false
value.converter.schemas.enable=false

database.history.kafka.bootstrap.servers=<kafka bootstrap servers, e.g., localhost:9092>
schema.history.internal.kafka.bootstrap.servers=<kafka bootstrap servers, e.g., localhost:9092>
EOT
)

#write connect-debezium-mysql.properties
cat << EOT >> $KAFKA_CONFIG/connect-debezium-mysql.properties
name=test-connector
connector.class=io.debezium.connector.mysql.MySqlConnector
database.hostname=<mysql host, e.g., 127.0.0.1>
database.port=<mysql port, e.g., 3306>
database.user=<mysql user>
database.password=<mysql password>
database.include.list=<which databse is needed to capture, e.g., ldbc>
table.include.list=<list the tables in the order of vertices, then edges>
snapshot.mode=<if enable buldload, set as "initial", otherwise set as "never">

$COMM_CONFIG

EOT

#write connect-debezium-postgresql.properties
cat << EOT >> $KAFKA_CONFIG/connect-debezium-postgresql.properties
name=test-connector
connector.class=io.debezium.connector.postgresql.PostgresConnector
database.hostname=<postgresql host, e.g., 127.0.0.1>
database.port=<postgresql port, e.g., 5432>
database.user=<postgresql user>
database.password=<postgresql password>
database.dbname=<which databse is needed to capture, e.g., ldbc>
table.include.list=<list the tables in the order of vertices, then edges>
snapshot.mode=<if enable buldload, set as "always", otherwise set as "never">

slot.name=debezium
plugin.name=pgoutput
publication.autocreate.mode=filtered

$COMM_CONFIG

EOT

#write server.properties
cat << EOT >> $KAFKA_CONFIG/server.properties
listeners=PLAINTEXT://localhost:9092
advertised.listeners=PLAINTEXT://localhost:9092

EOT
