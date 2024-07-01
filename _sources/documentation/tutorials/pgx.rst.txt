PostgreSQL Extension
=========================

GART is supported as a PostgreSQL Extension. It is possible to operate the GART service through a PostgreSQL front-end such as ``psql``.

Extension Installation
-----------------------

You need to install the PostgreSQL plugin by copying the shared library to the PostgreSQL library directory:

.. code:: bash
    :linenos:

    gart-env$ cd /workspace/gart/apps/pgx/
    gart-env$ make USE_PGXS=1 -j
    gart-env$ sudo make install

To enter commands into PostgreSQL, you need access to the PostgreSQL CLI. You can do this by opening a terminal on Unix-like systems or a command prompt on Windows, and running:

.. code:: bash

    # choose the `ldbc` database
    sudo -u postgres psql -d ldbc

    # or
    psql -U postgres -d ldbc


Extension Configuration
------------------------

After successfully logging in to your database, run the following command to create the ``gart`` extension:

.. code:: postgresql

    CREATE EXTENSION gart;

The ``gart`` extension has the following configuration options:

.. code:: ini
    :linenos:

    " The path of Kafka home and GART home
    [path]
    KAFKA_HOME=/path/to/kafka
    GART_HOME=/path/to/gart

    " The path of the log file and real-time log file
    " Log file is used to record the log of the GART service after the service is started and stopped
    " Real-time log file is used to record the log of the GART service in real time
    [log]
    log_path=/opt/postgresql/tmp.log
    real_time_log_path=/opt/postgresql/gart.log

    " The configuration of the GART service, as the arguments of the `gart` script
    [gart]
    db-type=postgresql
    rgmapping-file=/opt/postgresql/rgmapping-ldbc.yaml
    v6d-sock=/path/to/v6d.sock
    etcd-endpoints=127.0.0.1:23760
    etcd-prefix=gart_meta_
    subgraph-num=1
    enable-bulkload=1

Users can modify the configuration file to change the configuration of the GART service. The configuration file can be loaded as follows:

.. code:: postgresql

    SELECT * FROM gart_set_config('/path/to/config.ini');

Extension Usage
---------------

Connection Building
~~~~~~~~~~~~~~~~~~~

.. code:: postgresql
    :linenos:

    -- Pull up the GART service under the current database with the current user
    SELECT * FROM gart_get_connection('password');

    -- Get the latest read-only epoch
    SELECT * FROM gart_get_latest_epoch();

    -- Exit GART server
    SELECT * FROM gart_release_connection();

RGMapping Definition
~~~~~~~~~~~~~~~~~~~~~

We go through the SQL/PGQL language for the schema of the graph and the correspondence with the relational data source.

.. code:: postgresql
    :linenos:

    SELECT *
    FROM gart_define_graph($$
                            CREATE TABLE Person (
                                p_id INT PRIMARY KEY,
                                p_name VARCHAR(100)
                            )
                        $$);

Graph Analytical Processing
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Graph algorithms:**

.. code:: postgresql
    :linenos:

    -- Initialize the graph server and return the server ID
    SELECT * FROM gart_launch_graph_server('host', port);

    -- Retrieve information about the graph server
    SELECT * FROM gart_show_graph_server_info();

    -- Terminate the server operation
    SELECT * FROM gart_stop_graph_server(server_id);

    -- Execute the Single Source Shortest Path query
    SELECT * FROM gart_run_sssp(server_id, source_node);

**Graph traversal:**

.. code:: postgresql
    :linenos:

    SELECT *
    FROM gart_query_by_gremlin($$
                            g.V().count()
                            $$);
