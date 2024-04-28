.. _gstore:

Dynamic Graph Storage
=======

To enable dynamic graph data analysis of data that is changing, GART provides a dynamic graph store. The execution engine can read the latest data **snapshot** on this storage for real-time data analysis.

To balance performance, GART's graph store is implemented based on snapshots. Conceptually, the user can assume that the system will generate a complete snapshot of the graph data at regular intervals (default is 50 milliseconds). Note that the optimization techniques provided by GART do not create a full copy of the data every time.

In GART, we refer to the time interval between snapshots as the **epoch**, and each snapshot version will be identified by the epoch number.
GART manages all the metadata stored in `etcd`_, so you can get the current latest epoch via a command from `etcd`_, for example:

.. code:: bash

    # Identifies the latest readable epoch number
    # For which the machine ID is 0 and the data partition is 1
    etcdctl get --prefix "gart_meta_gart_blob_m0_p1_e" --keys-only


.. _etcd: https://etcd.io/
