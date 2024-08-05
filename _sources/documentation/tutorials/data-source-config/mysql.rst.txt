MySQL
================

Modify Configuration Files
--------------------------

MySQL configuration file ``/etc/mysql/my.cnf``:

.. code:: ini
    :linenos:

    [mysqld]
    # Prefix of the binlogs
    log-bin=mysql-bin

    # Binlog Format: row-based logging, only Maxwell needs binlog_format=row
    binlog_format=row
    binlog_row_image=full

    # Enable GTID for consistency
    gtid_mode=ON
    enforce_gtid_consistency=ON

    # The databases captured. GART will capture all databases if not specified.
    binlog-do-db=ldbc  # change the name to your database
    binlog-do-db=...   # change the name to your database

Create MySQL User
------------------

Create a MySQL user for the log capture (`Debezium`_):

.. code:: mysql
    :linenos:

    # Create a user call "dbuser" with password "123456"
    # The hostname part of the account name, if omitted, defaults to '%'.
    CREATE USER 'dbuser'@'localhost' IDENTIFIED BY '123456';

    # Grant necesarry privileges
    # PrivilegesRELOAD and SHOW DATABASES are only used for Debezium
    GRANT SELECT, RELOAD, SHOW DATABASES, REPLICATION SLAVE, REPLICATION CLIENT ON *.* TO 'dbuser'@'localhost';

    # Grant privileges on the database "dbuser", only used for Maxwell
    GRANT ALL ON maxwell.* TO 'dbuser'@'localhost';

.. _Debezium: https://debezium.io/documentation/reference/stable/connectors/mysql.html#mysql-creating-user
