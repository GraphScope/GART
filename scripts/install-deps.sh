#!/bin/bash

# Function to show usage
show_usage() {
  echo "Usage: $0 [path] [role]"
  echo "If no path is specified, './_deps' will be used as the default directory."
  echo "If no role is specified, dependencies required by all roles in GART will be installed."
}

# Function to create a directory if it doesn't exist
create_directory() {
  local dir=$1
  if [[ ! -d "$dir" ]]; then
    echo "Directory '$dir' does not exist. Attempting to create it..."
    mkdir -p "$dir"
    if [[ $? -ne 0 ]]; then
      echo "Error: Unable to create directory '$dir'."
      return 1
    fi
  fi
  return 0
}

if [[ $# -gt 2 ]]; then
    echo "Error: Too many arguments provided."
    show_usage
    exit 1 # Use 'return' instead of 'exit' when inside a function
fi

# Save the current working directory
ORIGINAL_DIR=$(pwd)

# Use the default directory if no argument is provided; display a warning
TARGET_DIR="$1"
if [[ -z "$TARGET_DIR" ]]; then
    echo "WARNING: No path provided, using default directory './_deps'."
    TARGET_DIR="./_deps"
fi

ROLE="$2"
if [[ -z "$ROLE" ]]; then
    echo "No role provided, installing all dependencies for GART."
    ROLE="All"
fi

# Check for the --help flag before invoking main
if [[ "$TARGET_DIR" == "--help" ]]; then
    show_usage
    exit 0
fi

# Ensure the TARGET_DIR exists or create it
create_directory "$TARGET_DIR" || exit 1

# Try to change to the target directory
cd "$TARGET_DIR" 2>/dev/null
if [[ $? -ne 0 ]]; then
    echo "Error: Unable to change to directory '$TARGET_DIR'."
    exit 2
fi

# Your commands here...
echo "Now in directory $(pwd). Executing commands..."

sudo apt update

sudo apt-get install -y build-essential cmake python3 python3-pip lsb-release wget
sudo apt-get install -y etcd

if [ "$ROLE" == "All" ]; then
    if ! command -v javac &> /dev/null; then
        echo "JDK not installed. Installing JDK..."
        sudo apt-get install -y default-jdk
    fi
fi

if [ "$ROLE" == "All" ] || [ "$ROLE" == "Analyzer" ]; then
    sudo apt-get install -y libmsgpack-dev
fi

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

if [ "$ROLE" == "All" ] || [ "$ROLE" == "Writer" ]; then
  # TBB
  git clone https://github.com/oneapi-src/oneTBB.git
  cd oneTBB
  mkdir -p build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release -DTBB_TEST=OFF
  make -j && sudo make install
  cd ../..
fi

if [ "$ROLE" != "Analyzer" ]; then
  # yaml-cpp
  git clone https://github.com/jbeder/yaml-cpp.git
  cd yaml-cpp
  mkdir -p build && cd build
  cmake -D BUILD_SHARED_LIBS=ON ..
  make -j && sudo make install
  cd ../..
fi

# pybind11
if [ "$ROLE" == "All" ] || [ "$ROLE" == "Analyzer" ]; then
  git clone https://github.com/pybind/pybind11.git
  cd pybind11
  mkdir -p build && cd build
  cmake .. -DPYBIND11_TEST=OFF
  make -j && sudo make install
  cd ../..
fi

if [ "$ROLE" != "Analyzer" ]; then
  # YAML for Python
  pip3 install pyyaml
fi

if [ "$ROLE" != "Analyzer" ]; then
  # librdkafka
  sudo apt-get install -y librdkafka-dev
fi

# Install sqlalchemy, pymysql, psycopg2
# for psycopg2, you need to install libpq-dev

sudo apt-get install -y libpq-dev
pip3 install sqlalchemy pymysql psycopg2 etcd3 libclang

if [ "$ROLE" == "All" ]; then
  # Install requirements-dev.txt for docs
  SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
  REQUIREMENTS_FILE="$SCRIPT_DIR/../requirements-dev.txt"
  pip3 install -r $REQUIREMENTS_FILE
fi

# vineyard
sudo apt-get install -y ca-certificates \
                libboost-all-dev \
                libcurl4-openssl-dev \
                openmpi-bin \
                libopenmpi-dev \
                libssl-dev \
                libunwind-dev \
                libz-dev

wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
sudo apt install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
sudo apt update
sudo apt install -y libarrow-dev=14.0.1-1 \
                    libarrow-dataset-dev=14.0.1-1 \
                    libarrow-acero-dev=14.0.1-1 \
                    libarrow-flight-dev=14.0.1-1 \
                    libgandiva-dev=14.0.1-1 \
                    libparquet-dev=14.0.1-1

git clone https://github.com/v6d-io/v6d
cd v6d
git submodule update --init
mkdir -p build && cd build
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/usr/local/lib64:/usr/local/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu
if [ "$ROLE" == "All" ] || [ "$ROLE" == "Writer" ] || [ "$ROLE" == "Analyzer" ]; then
  cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_VINEYARD_TESTS=OFF -DBUILD_VINEYARD_BENCHMARKS=OFF -DBUILD_SHARED_LIBS=ON -DBUILD_VINEYARD_LLM_CACHE=OFF -DBUILD_VINEYARD_BENCHMARKS=OFF -DBUILD_VINEYARD_IO=OFF -DBUILD_VINEYARD_PYTHON_BINDINGS=OFF
  make -j && sudo make install
  strip --strip-unneeded /usr/local/lib/libvineyard_graph.so
fi

if [ "$ROLE" == "Converter" ]; then
  cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_VINEYARD_TESTS=OFF -DBUILD_VINEYARD_BENCHMARKS=OFF -DBUILD_SHARED_LIBS=ON -DBUILD_VINEYARD_LLM_CACHE=OFF -DBUILD_VINEYARD_BENCHMARKS=OFF -DBUILD_VINEYARD_SERVER=OFF -DBUILD_VINEYARD_PYTHON_BINDINGS=OFF -DBUILD_VINEYARD_BASIC=OFF -DBUILD_VINEYARD_IO=OFF -DBUILD_VINEYARD_GRAPH=OFF
  make -j && sudo make install
fi
cd ../..

if [ "$ROLE" == "All" ] || [ "$ROLE" == "Writer" ] || [ "$ROLE" == "Analyzer" ]; then
  # libgrape-lite
  git clone https://github.com/alibaba/libgrape-lite.git
  cd libgrape-lite
  mkdir -p build && cd build
  cmake .. -DBUILD_LIBGRAPELITE_TESTS=OFF
  make -j && sudo make install
  cd ../..
fi

if [ "$ROLE" == "All" ]; then
  # pgql-lang
  git clone https://github.com/oracle/pgql-lang.git
  (cd pgql-lang; sh install.sh)
fi

if [ "$ROLE" == "All" ] || [ "$ROLE" == "Analyzer" ]; then
  # rapidjson
  sudo apt-get install -y rapidjson-dev
fi

pip3 install grpcio grpcio-tools mypy-protobuf requests paramiko

if [ "$ROLE" == "All" ] || [ "$ROLE" == "Analyzer" ]; then
  # required python modules
  pip3 install msgpack networkx
fi

if [ "$ROLE" == "All" ]; then
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
  ln -s $KAFKA_PLUGIN/*.jar $KAFKA_HOME/libs/ || true

COMM_CONFIG=$(cat << EOT
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

# write connect-debezium-mysql.properties
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

# write connect-debezium-postgresql.properties
cat << EOT >> $KAFKA_CONFIG/connect-debezium-postgresql.properties
name=test-connector
connector.class=io.debezium.connector.postgresql.PostgresConnector
database.hostname=<postgresql host, e.g., 127.0.0.1>
database.port=<postgresql port, e.g., 5432>
database.user=<postgresql user>
database.password=<postgresql password>
database.dbname=<which database is needed to capture, e.g., ldbc>
table.include.list=<list the tables in the order of vertices, then edges>
snapshot.mode=<if enable bulkload, set as "always", otherwise set as "never">

slot.name=debezium_0
plugin.name=pgoutput
publication.autocreate.mode=filtered

$COMM_CONFIG

EOT

# write server.properties
cat << EOT >> $KAFKA_CONFIG/server.properties
listeners=PLAINTEXT://0.0.0.0:9092
advertised.listeners=PLAINTEXT://localhost:9092

EOT
fi

rm -rf /deps/cpprestsdk /deps/etcd-cpp-apiv3 /deps/libgrape-lite /deps/oneTBB /deps/pgql-lang /deps/v6d /deps/yaml-cpp /deps/pybind11
rm -rf /var/lib/apt/lists/*
