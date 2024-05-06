.. _getting-started:

Quick Start
===============

Install GART
-------------------

GART currently requires installation via source code. Its dependencies and environment deployment can be done via Docker. The following steps will guide you through the installation process.

.. code:: bash
    :linenos:

    $ git clone git@github.com:GraphScope/GART.git
    $ cd GART
    $ docker build . -t gart-env
    $ docker run -it gart-env

After entering the docker image, build GART:

.. code:: bash
    :linenos:

    # In the docker image
    gart-env$ cd /workspace/gart/
    gart-env$ mkdir build; cd build
    gart-env$ cmake .. -DADD_GAE_ENGINE=ON
    gart-env$ make -j

Configure Data Source
----------------------------

Since the capture of logs requires corresponding permissions to the database, some configuration of the relational database, connection permissions, etc. is required.

In the Docker image we provided, the basic configuration of the database has been done and there is a user as ``dbuser`` with default password as ``123456``.

Initialize the PostgreSQL database by `LDBC-SNB`_ schema:

.. code:: bash

    gart-env$ cd /workspace/gart/
    gart-env$ ./apps/rdbms/init_schema.py --user dbuser --password 123456 --db ldbc

.. panels::
   :header: text-center
   :column: col-lg-12 p-2

   .. link-button:: ../../tutorials/data-source-config
      :type: ref
      :text: Tutorial
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Detailed instructions for configuring different data sources.

Configure Log Capturer
----------------------------

Configure Kafka (``$KAFKA_HOME/config/server.properties``) as follows:

.. code:: ini

    delete.topic.enable=true

We also need to set up a configuration of Debezium. Please replace the fields in the configuration file (``$KAFKA_HOME/config/connect-debezium-{mysql,postgresql}.properties``) that have sharp brackets (``<>``) with the actual contents (e.g., ``database.user``, ``database.password``).

Launch GART Server
----------------------------

GART offers two ways to start up, and you can choose one of the following two ways.

Launch as a standalone server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can launch GART by the ``gart`` script under the ``build`` directory, like:

.. code:: bash

    gart-env$ cd /workspace/gart/build/
    gart-env$ ./gart --user dbuser --password 123456

The arguments of ``--user`` and ``--password`` are the user name and the password in the database.

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
    :linenos:

    CREATE EXTENSION gart;

    SELECT * FROM gart_set_config('/workspace/gart/apps/pgx/gart-pgx-config-template.ini');

    \i /workspace/gart/vegito/test/schema/rgmapping-ldbc.sql

    SELECT * FROM gart_get_connection('123456');

Run Dynamic Graph Analysis
----------------------------

GART can create a fresh snapshot of a graph on real-time updated relational data. Users can perform graph analytic processing on this snapshot.

Initiate data updates
^^^^^^^^^^^^^^^^^^^^^^^^^^^

First, the data changes are modeled in the following way, in this case, the insertion of data:

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

- **Support for more data sources.** GART can support more data sources, such as MySQL, and PostgreSQL. Users can also implement their own data source by following the data source interface.

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

   The design of RGMapping and the interface to use it.

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
