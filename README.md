# GART: Graph Analysis on Relational Transactional Datasets

GART is a graph extension that includes an interface to an RDBMS and a dynamic graph store for online graph computation. It is designed to bridge the gap between relational OLTP and graph-based OLAP.

Please to refer [GART documentation](https://graphscope.github.io/GART) for more details.

## Table of Contents
- [What is GART](#what-is-gart)
- [Features](#features)
    - [Transparent Data Model Conversion](#transparent-data-model-conversion)
    - [Efficient Dynamic Graph Storage](#efficient-dynamic-graph-storage)
    - [Service-Oriented Deployment Model](#service-oriented-deployment-model)
- [Getting Started](#getting-started)
    - [Requirements](#requirements)
    - [Run GART](#run-gart)
- [License](#license)
- [Publications](#publications)

## What is GART

We would like to be able to use graph data flexibly without re-altering the existing relational database system. Moreover, users do not need to be aware of the storage of graph data and the synchronization of data between relational data and graph data for freshness. To fulfill this requirement, we build GART, an in-memory system for real-time online graph computation.

GART uses transactional logs (e.g., binlog) to capture data changes, then recovers data changes into fresh graph data in real time. GART integrates graph computation engines (e.g. GraphScope, NetworkX) to support efficient graph computation processing. The workflow of GART is shown below.

![](docs/images/arch.png)

- **1. Preprocess (Capture & Parser)**:
GART captures data changes from data sources by logs (e.g., Binlogs in SQL systems). Then, it parsers these logs into a recognized format, called as TxnLog. Currently, we use [Debezium](https://debezium.io/) (for MySQL, PostgreSQL, ...) as the log capture.

    The sample format of an inserted tuple of TxnLog is as follows (Debezium style, only necessary information):
  ```
  {
    "before": null,
    "after": {
        "org_id": "0",
        "org_type": "company",
        "org_name": "Kam_Air",
        "org_url": "http://dbpedia.org/resource/Kam_Air"
    },
    "source": {
        "ts_ms": 1689159703811,
        "db": "ldbc",
        "table": "organisation"
    },
    "op": "c"
  }
  ```
  This sample records the log that inserts a tuple of `organisation`.

- **2. Model Convert (RGMapping Converter)**:
This step is an important step for GART. The conversion between different data models for online graph computation requires more semantic information.
For example, it needs the mapping between relational tables and vertex/edge types, and the mapping between relational attributes and vertex/edge properties.
The GART administrator (such as DBA) can define the rules of relation-graph mapping (RGMapping) once by the interfaces provided by GART.
GART will convert relational data changes into graph data changes in the *unified logs* (UnifiedLog) automatically.

- **3. Graph Store (Dynamic GStore)**:
GART applies the graph data changes on the graph store. The graph store is dynamic, which means the writes from GART and the reads from the graph analysis processing can be executed on the store concurrently.

## Features

Compared to current solutions that provide graph interfaces on relational data, GART has three main features:

### Transparent Data Model Conversion
To adapt to rich workload flexibility, GART proposes transparent data model conversion by graph extraction interfaces, which define rules of relational-graph mapping.

We provide a sample definition file called [rgmapping-ldbc.yaml](vegito/test/schema/rgmapping-ldbc.yaml).

### Efficient Dynamic Graph Storage
To ensure the performance of graph analytical processing (GAP), GART proposes an efficient dynamic graph storage with good locality that stems from key insights into online graph computation, including:
1. an efficient and mutable compressed sparse row (CSR) representation to guarantee the locality of scanning edges;
2. a coarse-grained MVCC to reduce the temporal and spatial overhead of versioning;
3. a flexible property storage to efficiently run various GAP workloads.

Please refer to [our paper](https://www.usenix.org/conference/atc23/presentation/shen) for specific technical implementation details.

### Service-Oriented Deployment Model
GART acts as a service to synchronize database changes to the graph store.
When pulled up as a service on its own, users can try out the full power of GART and different graph computation engines on the graph store.
At the same time, GART also provides a front-end, used as a database plug-in, currently supported as PostgreSQL extension.
Users can invoke GART's functions in the database client, such as RGMapping definitions, graph computation on the graph store, etc.

## Getting Started

### Requirements

- [glog](https://github.com/google/glog), [gflags](https://github.com/gflags/gflags)
- [etcd-cpp-apiv3](https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3)
- [TBB](https://github.com/oneapi-src/oneTBB)
- [librdkafka](https://github.com/confluentinc/librdkafka)
- [Vineyard](https://github.com/v6d-io/v6d)
- [Apach Kafka](https://kafka.apache.org/quickstart)
- [Debezium](https://github.com/debezium/debezium)

### Run GART

Please to refer our [documentation](https://graphscope.github.io/GART/documentation/getting-started/quick-start.html).

## License

GART is released under [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0). Please note that third-party libraries may not have the same license as GART.


## Publications

[**USENIX ATC' 23**] [Bridging the Gap between Relational OLTP and Graph-based OLAP](https://www.usenix.org/conference/atc23/presentation/shen). Sijie Shen, Zihang Yao, Lin Shi, Lei Wang, Longbin Lai, Qian Tao, Li Su, Rong Chen, Wenyuan Yu, Haibo Chen, Binyu Zang, Jingren Zhou. USENIX Annual Technical Conference, Boston, MA, USA, July 2023.
