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

You can launch GART by the `gart` script under the `build` directory, like:
```
./gart --user [username-in-DB] --password [password]
```

The arguments of `--user` and `--password` is the user name and the password in the database for Maxwell.

The full usage of `gart` can be shown as:

```
./gart --help
```

## License

GART is released under [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0). Please note that third-party libraries may not have the same license as GraphScope.


## Publications

[**USENIX ATC' 23**] [Bridging the Gap between Relational OLTP and Graph-based OLAP](https://www.usenix.org/conference/atc23/presentation/shen). Sijie Shen, Zihang Yao, Lin Shi, Lei Wang, Longbin Lai, Qian Tao, Li Su, Rong Chen, Wenyuan Yu, Haibo Chen, Binyu Zang, Jingren Zhou. USENIX Annual Technical Conference, Boston, MA, USA, July 2023.