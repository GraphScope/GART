.. _architecture-of-gart:

Architecture
============

Overview and Workflow
------------------------

The following figure illustrates the architecture of GART.

.. figure:: /images/arch.png
   :width: 100%
   :alt: Architecture of GART

   Architecture of GART

To convert data logs from different *relational* data sources and update the real-time graph data, GART is architecturally divided into three main layers:

Pre-processing
^^^^^^^^^^^^^^

The capturer is responsible for capturing logs from different data sources (need to get the corresponding permissions of the data source, such as slave permissions in MySQL), and Parser needs to convert the binlog format of different data sources into a unified format of transactional log (TxnLog).
It is responsible for converting primary keys in relational data to IDs of vertices or edges in the graph.
It also ensures that in cases where logs may be out of order (distributed logs, streaming data, etc.), the logs are put in order for data recovery, ensuring the consistency of the graph data.

GART captures data changes from data sources by logs (e.g., Binlogs in SQL systems). Then, it parsers these logs into a recognized format, called as TxnLog. Currently, we use `Debezium`_ as the log capture. The sample format of an inserted tuple of TxnLog is as follows (Debezium style, only necessary information):

.. code-block:: json

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

This sample records the log that inserts a tuple of ``organisation``. The ``before`` field is null, indicating that this is an insert operation. The ``after`` field records the inserted tuple. The ``source`` field records the source of the log, including the timestamp, database, and table. The ``op`` field records the operation type.

RGMapping: Model Mapping
^^^^^^^^^^^^^^^^^^^^^^^^^^

The log capturer in GART is responsible for capturing logs from the data source.
GART needs to get the corresponding permissions of the data source of the data source through user configuration, such as slave permissions in MySQL.
The log parser then converts the *binlog* format of the different data sources into the transaction log (TxnLog) format in the prescribed format.

Dynamic Graph Storage
^^^^^^^^^^^^^^^^^^^^^^^^^^

It is responsible for updating the graphics data and providing a unified graphics storage interface to the upper-tier graphics computing engine.
The interface provided by the graph store to the execution engine is encapsulated through the `GRIN`_ library.
The storage layer provides snapshots of the graph data, allowing users to analyze the graph in real time as if it were a static graph.
GART's dynamic graph store is based on `Vineyard`_, which simplifies access to shared data by different processes.
In a distributed scenario, the preprocessing and model transformation layers are deployed on a single machine, and the transformed UnifiedLog is sent to data stores on different machines according to distributed distribution rules.

Core Features
-------------

Transparent Data Model Conversion
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To adapt to rich workload flexibility, GART proposes transparent data model conversion by graph extraction interfaces, which define rules of relational-graph mapping.
During the use of GART, data changes from relational databases are converted into graph data updates based on user-defined model mapping rules (RGMapping).  GART provides a set of interfaces for DBAs to define data model conversion rules, which can be compatible with `SQL/PGQ`_ DDL.

We provide a sample definition file called `RGMappings`_.

Efficient Dynamic Graph Storage
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To ensure the performance of graph computation, GART proposes an efficient dynamic graph storage with good locality that stems from key insights into online graph computation workloads, including:

1. an efficient and mutable compressed sparse row (CSR) representation to guarantee the locality of scanning edges;

2. a coarse-grained MVCC to reduce the temporal and spatial overhead of versioning;

3. flexible property storage to efficiently run various graph computation workloads.

Service-Oriented Deployment Model
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

GART acts as a service to synchronize database changes to the graph store.
When pulled up as a service on its own, users can try out the full power of GART and different graph computation engines on the graph store.
At the same time, GART also provides a front-end, used as a database plug-in, currently supported as a PostgreSQL extension.
Users can invoke GART's functions in the database client, such as RGMapping definitions, graph computation on the graph store, etc.

.. _GRIN: https://graphscope.io/docs/latest/storage_engine/grin/
.. _RGMappings: https://github.com/GraphScope/GART/blob/main/vegito/test/schema/rgmapping-ldbc.sql
.. _Vineyard: https://v6d.io/
.. _SQL/PGQ: https://pgql-lang.org/
.. _Debezium: https://debezium.io/
