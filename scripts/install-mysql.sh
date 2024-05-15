#!/bin/bash

sudo DEBIAN_FRONTEND=noninteractive apt install -y mysql-server
pip3 install pymysql cryptography

sudo tee -a /etc/mysql/my.cnf << EOT

[mysqld]
# Prefix of the binlogs
log-bin=mysql-bin

# Binlog Format: row-based logging, maxwell needs binlog_format=row
binlog_format=row
binlog_row_image=full

# The databases captured. GART will capture all databases if not specified.
binlog-do-db=ldbc  # change the name to your database

EOT

sudo service mysql restart
