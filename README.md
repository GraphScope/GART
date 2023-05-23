# GART

GART is an in-memory system extended from HTAP systems for hybrid transactional and graph analytical processing (HTGAP).

## What is GART

[TBD]

## Features

GART should fulfill two unique goals not encountered by HTAP systems.

### Transparent Data Model Conversion
To adapt to rich workloads flexibility, GART proposes transparent data model conversion by graph extraction interfaces, which define rules of relational-graph mapping.

### Efficient Dynamic Graph Storage
To ensure the performance of graph analytical processing (GAP), GART proposes an efficient dynamic graph storage with good locality that stems from key insights into HTGAP workloads, including:
1. an efficient and mutable compressed sparse row (CSR) representation to guarantee the locality of scanning edges;
2. a coarse-grained MVCC to reduce the temporal and spatial overhead of versioning;
3. a flexible property storage to efficiently run various GAP workloads.

## Deployment

### Requirements

- [glog](https://github.com/google/glog), [gflags](https://github.com/gflags/gflags)
- [etcd-cpp-apiv3](https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3)
- [TBB](https://github.com/oneapi-src/oneTBB)
- [librdkafka](https://github.com/confluentinc/librdkafka)
- [Vineyard](https://github.com/v6d-io/v6d)
- [Apach Kafka](https://kafka.apache.org/quickstart)
- [Maxwell](https://github.com/zendesk/maxwell)

### Building from source
```shell
git clone https://github.com/GraphScope/GART.git gart
cd gart

mkdir build; cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

## Getting Started

### Configure Data Source

Before running GART, we need to configure the data source to capture its logs.
Take MySQL as an example.

- Kafka configuration file `$KAFKA_HOME/config/server.properties`
    ```
    delete.topic.enable=true
    ```

- MySQL configuration file `/etc/mysql/my.cnf`:
    ```
    # Prefix of the binlogs
    log-bin=mysql-bin

    # Binlog Format: row-based logging, maxwell needs binlog_format=row
    binlog_format=row

    # The databases captured. GART will capture all databases if not specified.
    binlog-do-db=ldbc  # change the name to your database
    binlog-do-db=...   # change the name to your database
    ```

- Create a MySQL user for the log capturer ([Maxwell](https://github.com/zendesk/maxwell/blob/master/docs/docs/quickstart.md))
    ```
    # Create a user call "maxwell" with password "123456"
    # The host name part of the account name, if omitted, defaults to '%'.
    CREATE USER 'maxwell'@'localhost' IDENTIFIED BY '123456';

    # Grant replication and read-only privileges
    GRANT SELECT, REPLICATION CLIENT, REPLICATION SLAVE ON *.* TO 'maxwell'@'localhost';

    # Grant privileges on the database "maxwell"
    GRANT ALL ON maxwell.* TO 'maxwell'@'localhost';
    ```

### Run GART

You can launch GART by the `gart` script under the `build` directory, like:
```
export KAFKA_HOME=xxx
export MAXWELL_HOME=xxx
./gart --user maxwell --password 123456
```

The arguments of `--user` and `--password` is the user name and the password in the database for Maxwell.

The full usage of `gart` can be shown as:

```
./gart --help
```

### Mirco Demo
- Download test datasets (use `ldbc_sample`)
    ```
    git clone https://github.com/GraphScope/gstest.git
    ```

- Initialize database schema in MySQL (need a user with necessary privileges)
    ```
    pip3 install pymysql cryptography

    cd gart
    ./apps/mysql/init_scehma.py --user xxx --password xxx --db ldbc
    ```

    If you have no such user, you can create the user before running `init_scehma.py` like:
    ```
    CREATE USER test IDENTIFIED BY '123456';
    GRANT SELECT, CREATE, DROP, INSERT, DELETE ON ldbc.* TO test;
    ```

- Lanch GART
    ```
    export KAFKA_HOME=xxx
    export MAXWELL_HOME=xxx

    cd build
    ./gart --user maxwell --password 123456 --db-name ldbc
    ```

- Start transactional data insertion
    ```
    ./insert_db.py --user maxwell --password 123456 --db ldbc --data_dir [path to gstest/ldbc_sample]
    ```

- Start graph analysis
    ```
    ./apps/run_gart_app
    ```

## License

GART is released under [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0). Please note that third-party libraries may not have the same license as GraphScope.


## Publications

[**USENIX ATC' 23**] [Bridging the Gap between Relational OLTP and Graph-based OLAP](https://www.usenix.org/conference/atc23/presentation/shen). Sijie Shen, Zihang Yao, Lin Shi, Lei Wang, Longbin Lai, Qian Tao, Li Su, Rong Chen, Wenyuan Yu, Haibo Chen, Binyu Zang, Jingren Zhou. USENIX Annual Technical Conference, Boston, MA, USA, July 2023.