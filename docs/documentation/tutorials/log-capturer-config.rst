Log Capturer Configuration
==============================

This step is to allow GART's log capturer to fetch the logs of specified database tables to avoid causing irrelevant logs to be tracked.

Edit Kafka configuration (``$KAFKA_HOME/config/server.properties``) as follows:

.. code:: ini

    delete.topic.enable=true

Set up a configuration of Debezium. Please replace the fields in the configuration file (``$KAFKA_HOME/config/connect-debezium-{mysql,postgresql}.properties``) that have sharp brackets (``<>``) with the actual contents (e.g., ``database.user``, ``database.password``).
