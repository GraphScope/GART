.. _deploy-distributed:

Distributed Deployment
========================================

GART is designed to be deployed in a distributed manner.
This means that you can deploy different components of GART on different nodes of a cluster.
To achieve this, you need to provide a configuration file, and GART provides a Python script to launch GART on a cluster easily.


User Configuration
--------------------
Please ensure that the GART folder is accessible at the same path on all nodes of the cluster.
Then you can create a configuration file in the following format:

.. code:: json
    :linenos:

    {
        // Database port
        "db_host": "127.0.0.1",
        // Database port
        "db_port": 5432,
        // Database type: mysql or postgresql
        "db_type": "postgresql",
        // Database name
        "db_name": "ldbc",
        // Database user
        "db_user": "dbuser",
        // Database password
        "db_password": "123456",
        "rgmapping_file": "/path/to/rgmapping.yaml",
        "v6d_size": "750G",
        "etcd_prefix": "gart_meta_",
        // entrypoint of the kafka server
        "kafka_server": "127.0.0.1:9092",
        "total_subgraph_num": 2,
        // path to the GART build folder
        "gart_bin_path": "/path/to/GART/build/",
        // whether to enable bulk load
        "enable_bulkload": 1,
        // where to launch the capturer
        "capturer_host": "127.0.0.1",
        // where to launch the converter
        "converter_host": "127.0.0.1",
        // path to kafka
        "kafka_path": "/path/to/kafka",
        "writer_hosts": [
            {
                // where to launch the writer to create subgraph 0
                "subgraph_id": 0,
                "host": "127.0.0.1"
            },
            {
                // where to launch the writer to create subgraph 1
                "subgraph_id": 1,
                "host": "127.0.0.1"
            }
        ]
    }

Launch GART with configuration
----------------------------------------
GART provides a Python script to launch GART on a cluster easily. You can launch GART with the following command by providing the configuration file in the previous step:

.. code:: bash

./path/to/GART/scripts/distributed_deployment.py --config_file /path/to/user_config.json
