.. _getting-started:

Getting Started
===============

Install GART
-------------------

GART currently requires installation via source code. Its dependencies and environment deployment can be done via docker

.. code:: bash

    $ git clone git@github.com:GraphScope/GART.git
    $ cd GART
    $ docker build . -t gart-env
    $ docker run -it gart-env

After entering the docker image, build GART:

.. code:: bash

    # In the docker image
    gart-env$ cd /workspace/gart/
    gart-env$ mkdir build; cd build
    gart-env$ cmake .. -DADD_GAE_ENGINE=ON
    gart-env$ make -j

Configure Data Source
----------------------------

In order to implement the conversion of relational data to graph data using GART, the data source needs to be configured with the necessary permissions. Here is an example of PostgreSQL.

The PostgreSQL configuration file is in the directory ``/etc/postgresql/$PSQL_VERSION/main/``. In our docker image, the PostgreSQL version is 16, so the configuration file is in the directory ``/etc/postgresql/16/main/``.

1. Modify the configuration file ``postgresql.conf`` to enable WAL as follows:

.. code:: ini

    wal_level = logical
    max_replication_slots = 1 # larger than 0
    max_wal_senders = 1 # larger than 0

2. Create a PostgreSQL user (``dbuser``) for the log capture Debezium:

.. code:: postgresql

    CREATE USER dbuser WITH PASSWORD '123456';
    ALTER USER dbuser REPLICATION;
    ALTER USER dbuser LOGIN;
    GRANT pg_read_server_files TO dbuser;       -- For loading CSV files

    CREATE DATABASE ldbc;
    GRANT ALL ON DATABASE ldbc TO dbuser;

    \c ldbc
    GRANT ALL ON SCHEMA public TO dbuser;

3. Modify the configuration file `/etc/postgresql/$PSQL_VERSION/main/pg_hba.conf` to `trust the user`_ ``dbuser``:

.. code:: none

    local   replication     dbuser                          trust
    host    replication     dbuser  127.0.0.1/32            trust
    host    replication     dbuser  ::1/128                 trust

4. Restart PostgreSQL:

.. code:: bash

    gart-env$ sudo /etc/init.d/postgresql restart

5. Initialize the PostgreSQL database by `LDBC-SNB`_ schema:

.. code:: bash

    gart-env$ cd /workspace/gart/
    gart-env$ ./apps/rdbms/init_schema.py --user dbuser --password 123456 --db ldbc

Launch GART Server
----------------------------

GART offers two ways to start up, and you can choose one of the following two ways.

Launch as a standalone server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can launch GART by the ``gart`` script under the ``build`` directory, like:

.. code:: bash

    gart-env$ cd /workspace/gart/build/
    gart-env$ ./gart --user dbuser --password 123456

The arguments of ``--user`` and ``--password`` is the user name and the password in the database.

The full usage of ``gart`` can be shown as:

.. code:: bash

    gart-env$ --help

You can stop GART by:

.. code:: bash

    gart-env$ ./stop-gart

Launch as PostgreSQL plugin
^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can launch GART as a PostgreSQL plugin:

.. code:: postgresql

    CREATE EXTENSION gart;

    SELECT * FROM gart_set_config('/workspace/gart/apps/pgx/gart-pgx-config-template.ini');

    \i /workspace/gart/vegito/test/schema/rgmapping-ldbc.sql

    SELECT * FROM gart_get_connection('123456');

Run Dynamic Graph Analysis
----------------------------

GART is able to create a fresh snapshot of a graph on a real-time updated relational data. Users are able to perform graph analytic processing on this snapshot.

Initiate data updates
^^^^^^^^^^^^^^^^^^^^^^^^^^^

First, the data changes are modeled in the following way, in this case the insertion of data:

.. code:: bash

    gart-env$ cd /workspace/gart/
    gart-env$ ./apps/rdbms/insert_db_txn.py --user dbuser --password 123456 --data_dir /workspace/gstest/ldbc_sample/

Then, the graph snapshot is created.

Launch the graph analysis
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The graph analysis can be launched by the following command:

.. code:: bash

    gart-env$ cd /workspace/gart/
    gart-env$ mpirun -n 1 ./apps/run_gart_app --read_epoch 0 --app_name sssp --sssp_source_label organisation --sssp_source_oid 0 --sssp_weight_name wa_work_from

Next steps
----------

GART also provides:

- **Distributed deployment based on Kubernetes.** GART can be deployed on a distributed environment based on Kubernetes.

- **Support for more data sources.** GART can support more data sources, such as MySQL, PostgreSQL. Users can also implement their own data source by following the data source interface.

- **Support for more graph analysis algorithms.** GART can support more graph analysis algorithms, such as PageRank, Connected Components, etc. The execution engine of GART is based on `GraphScope`_, which supports a wide range of graph algorithms. We also support `NetworkX`_ as the execution engine.

- **Flexible data model mapping.** GART can support more data model mappings by RGMapping. Users can implement their own mapping rules by following the RGMapping interface.

Learn more about key concepts of GART from the following user guides:

.. panels::
   :header: text-center
   :column: col-lg-12 p-2

   .. link-button:: architecture
      :type: ref
      :text: Architecture
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Overview of GART.

.. panels::
   :header: text-center
   :container: container-lg pb-4
   :column: col-lg-4 col-md-4 col-sm-4 col-xs-12 p-2
   :body: text-center

   .. link-button:: key-concepts/rgmapping
      :type: ref
      :text: RGMapping
      :classes: btn-block stretched-link

   The design of relational-graph mapping (RGMapping).

   ---

   .. link-button:: key-concepts/graph-storage
      :type: ref
      :text: Dynamic Graph Storage
      :classes: btn-block stretched-link

   The design of the dynamic graph storage in GART.

   ---

   .. link-button:: key-concepts/graph-server
      :type: ref
      :text: Graph Server
      :classes: btn-block stretched-link

   The design of the graph server in GART.

.. _trust the user: https://debezium.io/documentation/reference/stable/postgres-plugins.html#:~:text=pg_hba.conf%20%2C%20configuration%20file%20parameters%20settings

.. _LDBC-SNB: https://ldbcouncil.org/benchmarks/snb/

.. _GraphScope: https://github.com/alibaba/GraphScope

.. _NetworkX: https://networkx.org/
