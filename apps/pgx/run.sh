#! /usr/bin/bash

####################### Parse Arguments #######################
VALID_ARGS=$(getopt -o c:u:p:b:h? --long config-file:,user:,password:,db-name:,stop,help -- "$@")

config_file=
username=
password=
db_name=
stop=false

eval set -- "$VALID_ARGS"
while [ : ]; do
  case "$1" in
    -c | --config_file)
        config_file=$2
        shift 2
        ;;
    -b | --db-name)
        db_name=$2
        shift 2
        ;;
    -u | --user)
        username=$2
        shift 2
        ;;
    -p | --password)
        password=$2
        shift 2
        ;;
         --stop)     # hide from help
        stop=true
        shift
        ;;
    -h | --help | ?)
        echo "Usage: $0 [options]"
        echo "  -c, --config-file:     config file"
        echo "  -d, --db-name:         db name"
        echo "  -u, --user:            username"
        echo "  -p, --password:        password"
        echo "  --stop:                stop"
        echo "  -h, --help:            help (this message)"
        exit 0
        ;;
    --)
        shift;
        break
        ;;
  esac
done

parse_ini() {
    section=$1
    key=$2
    awk -F '=' -v section="$section" -v key="$key" '
        /^\['"$section"'\]$/ {found=1; next}
        /^\[/ && found {exit}
        found && $1 == key {print $2; exit}
        ' "$config_file" | tr -d ' '
}

KAFKA_HOME=$(parse_ini "path" "KAFKA_HOME" $config_file)
GART_HOME=$(parse_ini "path" "GART_HOME" $config_file)
v6d_sock=$(parse_ini "gart" "v6d-sock" $config_file)

echo "KAFKA_HOME: $KAFKA_HOME"
echo "GART_HOME: $GART_HOME"

if [ "$stop" = true ]; then
    echo "v6d_sock: $v6d_sock"
    (export KAFKA_HOME=$KAFKA_HOME; cd $GART_HOME/build; \
    ./stop-gart --kill-v6d-sock $v6d_sock)
    exit 0
fi

real_time_log_path=$(parse_ini "log" "real_time_log_path" $config_file)

echo "real_time_log_path: $real_time_log_path"

db_type=$(parse_ini "gart" "db-type" $config_file)
rgmapping=$(parse_ini "gart" "rgmapping-file" $config_file)
etcd_endpoints=$(parse_ini "gart" "etcd-endpoints" $config_file)
subgraph_num=$(parse_ini "gart" "subgraph-num" $config_file)
enable_bulkload=$(parse_ini "gart" "enable-bulkload" $config_file)

echo "username: $username"
echo "password: $password"
echo "db_type: $db_type"
echo "rgmapping: $rgmapping"
echo "v6d_sock: $v6d_sock"
echo "etcd_endpoints: $etcd_endpoints"
echo "subgraph_num: $subgraph_num"
echo "enable_bulkload: $enable_bulkload"

(export KAFKA_HOME=$KAFKA_HOME; cd $GART_HOME/build; \
./gart --db-type $db_type -u $username -p $password -b $db_name -r $rgmapping --v6d-sock $v6d_sock -e $etcd_endpoints --subgraph-num $subgraph_num --enable-bulkload $enable_bulkload 2>&1 \
 | tee $real_time_log_path &)
