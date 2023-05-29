#!/bin/bash

sudo apt install mysql-server
pip3 install pymysql cryptography

sudo cat << EOT >> greetings.txt
[mysqld]
# Prefix of the binlogs
log-bin=mysql-bin

# Binlog Format: row-based logging, maxwell needs binlog_format=row
binlog_format=row

# The databases captured. GART will capture all databases if not specified.
binlog-do-db=ldbc  # change the name to your database
EOT

sudo service mysql restart