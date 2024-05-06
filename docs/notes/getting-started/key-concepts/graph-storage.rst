.. _gstore:

Dynamic Graph Storage
============================

Graph Representation
--------------------

To enable dynamic graph data analysis of data that is changing, GART provides a dynamic graph store. The execution engine can read the latest data **snapshot** on this storage for real-time data analysis.

To balance performance, GART's graph store is implemented based on snapshots. Conceptually, the user can assume that the system will generate a complete snapshot of the graph data at regular intervals (default is 50 milliseconds). Note that the optimization techniques provided by GART do not create a full copy of the data every time.

Users can think of the graph data stored in GART as a multi-version *compressed sparse row* (CSR) representation.
Specific details of the design can be found in `our paper <https://www.usenix.org/system/files/atc23-shen.pdf>`_.
In GART, we refer to the time interval between snapshots as the **epoch**, and each snapshot version will be identified by the epoch number.

Distributed Store
--------------------

The graph data is stored in `Vineyard`_ in a distributed manner.
Vineyard (v6d) is an in-memory immutable data manager that offers out-of-the-box high-level abstraction and zero-copy sharing for distributed data in big data tasks.

Graph data is partitioned into multiple data partitions, called sub-graph, and each data partition is stored in a machine. A machine can hold multiple data partitions.
We can use the machine ID and data partition ID to identify a specific data partition.

Metadata Management
--------------------

GART manages all the metadata stored in `etcd`_, a distributed key-value store.
The metadata includes the epoch number, machine ID, and data partition ID for each snapshot.
Users can retrieve the metadata by querying the etcd store.

.. code:: bash

    # Identifies the latest readable epoch number
    # For which the machine ID is 0 and the data partition is 1
    etcdctl get --prefix "gart_meta_gart_blob_m0_p1_e" --keys-only


.. _Vineyard: https://v6d.io/
.. _etcd: https://etcd.io/
