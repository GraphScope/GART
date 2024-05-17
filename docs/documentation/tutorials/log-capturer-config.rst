Log Capturer Configuration
==============================

This step is to allow GART's log capturer to fetch the logs of specified database tables to avoid causing irrelevant logs to be tracked.

Edit Kafka configuration (``$KAFKA_HOME/config/server.properties``) as follows:

.. code:: ini

    delete.topic.enable=true

Set up a configuration of Debezium. Please replace the fields in the configuration file (``$KAFKA_HOME/config/connect-debezium-{mysql,postgresql}.properties``) that have sharp brackets (``<>``) with the actual contents (e.g., ``database.user``, ``database.password``).

The following are the parts of each configuration file that need to be changed.

For Postgresql
^^^^^^^^^^^^^^^^^

.. code:: ini
    :linenos:

    database.history.kafka.bootstrap.servers=<kafka bootstrap servers, e.g., localhost:9092>
    schema.history.internal.kafka.bootstrap.servers=<kafka bootstrap servers, e.g., localhost:9092>

    name=<name of the connector, e.g., test-connector>
    connector.class=io.debezium.connector.postgresql.PostgresConnector
    database.hostname=<postgresql host, e.g., 127.0.0.1>
    database.port=<postgresql port, e.g., 5432>
    database.user=<postgresql user>
    database.password=<postgresql password>
    database.dbname=<which database is needed to capture, e.g., ldbc>
    table.include.list=<list the tables in the order of vertices, then edges>
    snapshot.mode=<if enable bulkload, set as "always", otherwise set as "never">

    slot.name=debezium
    plugin.name=pgoutput
    publication.autocreate.mode=filtered

For MySQL
^^^^^^^^^^^^^^^^^

.. code:: ini
    :linenos:

    database.history.kafka.bootstrap.servers=<kafka bootstrap servers, e.g., localhost:9092>
    schema.history.internal.kafka.bootstrap.servers=<kafka bootstrap servers, e.g., localhost:9092>

    name=<name of the connector, e.g., test-connector>
    connector.class=io.debezium.connector.mysql.MySqlConnector
    database.hostname=<mysql host, e.g., 127.0.0.1>
    database.port=<mysql port, e.g., 3306>
    database.user=<mysql user>
    database.password=<mysql password>
    database.include.list=<which databse is needed to capture, e.g., ldbc>
    table.include.list=<list the tables in the order of vertices, then edges>
    snapshot.mode=<if enable buldload, set as "initial", otherwise set as "never">

Automatic Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To simplify this part of the configuration, we have provided scripts for the same.

.. code:: bash

    # In the docker image
    gart-env$ cd /workspace/gart/

    # Show the help information
    gart-env$ ./scripts/update_kafka_config_file.py --help
