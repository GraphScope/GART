PostgreSQL
================


To implement the conversion of relational data to graph data using GART, the data source needs to be configured with the necessary permissions. Here is an example of PostgreSQL.

The PostgreSQL configuration file is in the directory ``/etc/postgresql/$PSQL_VERSION/main/``. In our docker image, the PostgreSQL version is 16, so the configuration file is in the directory ``/etc/postgresql/16/main/``.

1. Modify the configuration file ``postgresql.conf`` to enable WAL as follows:

.. code:: ini

    wal_level = logical
    max_replication_slots = 10  # larger than 0, default is 10
    max_wal_senders = 10  # larger than 0, default is 10

2. Create a PostgreSQL user (``dbuser``) for the log capture Debezium:

.. code:: postgresql
    :linenos:

    CREATE USER dbuser WITH PASSWORD '123456';
    ALTER USER dbuser REPLICATION;
    ALTER USER dbuser LOGIN;
    GRANT pg_read_server_files TO dbuser;       -- For loading CSV files

    CREATE DATABASE ldbc;
    GRANT ALL ON DATABASE ldbc TO dbuser;

3. Switch to the database ``ldbc`` and create the schema ``public``:

.. code::

    \c ldbc

.. code:: postgresql

    GRANT ALL ON SCHEMA public TO dbuser;

4. Modify the configuration file ``/etc/postgresql/$PSQL_VERSION/main/pg_hba.conf`` to `trust the user`_ ``dbuser``:

.. code:: none

    local   replication     dbuser                          trust
    host    replication     dbuser  127.0.0.1/32            trust
    host    replication     dbuser  ::1/128                 trust

5. Restart PostgreSQL:

.. code:: bash

    gart-env$ sudo /etc/init.d/postgresql restart

.. _trust the user: https://debezium.io/documentation/reference/stable/postgres-plugins.html#:~:text=pg_hba.conf%20%2C%20configuration%20file%20parameters%20settings
