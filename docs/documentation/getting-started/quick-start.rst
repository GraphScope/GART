.. _getting-started:

Quick Start
===============

This section gives an experiential process of GART where we provide containers for deployment. Users can also try local deployment, we will give local deployment and usage tips.

Step 0: Get GART
-------------------

The following steps will guide you through the installation process.

GART currently requires installation via source code:


.. code:: bash

    $ git clone git@github.com:GraphScope/GART.git gart

Step 1: Start the GART Environment
--------------------------------------

We provide a Docker image that contains all the dependencies and configurations needed to run GART. The Docker image is based on Ubuntu 20.04.

.. tip::

    If you would like to install it locally, please refer to the `detailed deployment tutorial <../deployment/deploy-local.html>`_ we provide.

To build the Docker container called ``gart-env`` and enter the container:

.. code:: bash
    :linenos:

    $ cd gart
    $ docker build . -t gart-env
    $ docker run -it gart-env

After entering the Docker container, build GART:

.. code:: bash
    :linenos:

    # In the docker image
    gart-env$ cd /workspace/gart/
    gart-env$ mkdir build; cd build
    gart-env$ cmake .. -DADD_GAE_ENGINE=ON
    gart-env$ make -j

.. panels::
   :header: text-center
   :column: col-lg-12 p-2

   .. link-button:: ../deployment/deploy-local
      :type: ref
      :text: Deployment
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Detailed instructions for deploying GART locally.

Step 2: Configure Data Source
------------------------------------

Since GART needs the log access privileges of the data source to access the data logs in real time, it is necessary to configure the privileges at the data source first.

In the Docker image we provided, the basic configuration of the database has been done and there is a user as ``dbuser`` with default password as ``123456``.

.. tip::

    If you want to use a different user, you can create a new user in the database and grant the user the necessary privileges. Please refer to the `tutorial <../tutorials/data-source-config.html>`_ for more information.

Initialize the PostgreSQL database by `LDBC-SNB`_ schema:

.. code:: bash

    gart-env$ cd /workspace/gart/
    gart-env$ ./apps/rdbms/init_schema.py --user dbuser --password 123456 --db ldbc

.. panels::
   :header: text-center
   :column: col-lg-12 p-2

   .. link-button:: ../tutorials/data-source-config
      :type: ref
      :text: Tutorial
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Detailed instructions for configuring different data sources.

Step 3: Configure Log Capturer
---------------------------------

This step is to allow GART's log capturer to fetch the logs of specified database tables to avoid causing irrelevant logs to be tracked.

Edit Kafka configuration (``$KAFKA_HOME/config/server.properties``) as follows:

.. code:: ini

    delete.topic.enable=true

Set up a configuration of Debezium. Please replace the fields in the configuration file (``$KAFKA_HOME/config/connect-debezium-{mysql,postgresql}.properties``) that have sharp brackets (``<>``) with the actual contents (e.g., ``database.user``, ``database.password``).

Step 4: Launch GART Server
----------------------------

GART offers two ways to start up, and you can choose one of the following two ways.

Alternative #1: Launch as a standalone server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can launch GART by the ``gart`` script under the ``build`` directory, like:

.. code:: bash

    gart-env$ cd /workspace/gart/build/
    gart-env$ ./gart --user dbuser --password 123456

The arguments of ``--user`` and ``--password`` are the user name and the password in the database.

The full usage of ``gart`` can be shown as:

.. code:: bash

    gart-env$ ./gart --help

You can stop GART by:

.. code:: bash

    gart-env$ ./stop-gart

Alternative #2: Launch as PostgreSQL plugin
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can launch GART as a PostgreSQL plugin:

You need to install the PostgreSQL plugin by copying the shared library to the PostgreSQL library directory:

.. code:: bash
    :linenos:

    gart-env$ cd /workspace/gart/apps/pgx/
    gart-env$ make USE_PGXS=1 -j
    gart-env$ sudo make install

To configure and utilize the GART extension within your PostgreSQL environment, you'll need to follow these steps.

**1. Open the PostgreSQL Command Line Interface:**

To enter commands into PostgreSQL, you need access to the PostgreSQL CLI. You can do this by opening a terminal on Unix-like systems or a command prompt on Windows, and running:

.. code:: bash

    # choose the `ldbc` database
    sudo -u postgres psql -d ldbc

    # or
    psql -U postgres -d ldbc

**2. Install the ``gart`` Extension:**

After successfully logging in to your database, run the following command to create the ``gart`` extension:

.. code:: postgresql

    CREATE EXTENSION gart;

**3. Configure ``gart`` Extension:**

Now set up the ``gart`` configuration using the following command:

.. code:: postgresql

    SELECT * FROM gart_set_config('/workspace/gart/apps/pgx/gart-pgx-config-template.ini');

**4. Load the LDBC-SNB schema:**

.. code::

    \i /workspace/gart/vegito/test/schema/rgmapping-ldbc.sql

**5. Establish a Connection Using ``gart``:**

Finally, establish a connection using a specific identifier like so:

.. code:: postgresql

    SELECT * FROM gart_get_connection('123456');

Step 5: Run Dynamic Graph Analysis
-------------------------------------

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

- **Distributed deployment.** GART allows users to provide a simple configuration file, and it will run on a cluster.

- **Deployment based on Kubernetes.**  GART can be deployed on Kubernetes.

- **Support for more data sources.** GART can support more data sources, such as MySQL, and PostgreSQL. Users can also implement their own data source by following the data source interface.

- **Support for more graph analysis algorithms.** GART can support more graph analysis algorithms, such as PageRank, Connected Components, etc. The execution engine of GART is based on `GraphScope`_, which supports a wide range of graph algorithms. We also support `NetworkX`_ as the execution engine.

- **Flexible data model mapping.** GART can support more data model mappings by RGMapping. Users can implement their own mapping rules by following the RGMapping interface.

.. _trust the user: https://debezium.io/documentation/reference/stable/postgres-plugins.html#:~:text=pg_hba.conf%20%2C%20configuration%20file%20parameters%20settings

.. _LDBC-SNB: https://ldbcouncil.org/benchmarks/snb/

.. _GraphScope: https://github.com/alibaba/GraphScope

.. _NetworkX: https://networkx.org/
