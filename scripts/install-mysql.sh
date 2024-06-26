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

# Enable GTID for consistency
gtid_mode=ON
enforce_gtid_consistency=ON

# The databases captured. GART will capture all databases if not specified.
binlog-do-db=ldbc  # change the name to your database

EOT

# For some reason, the mysql user's home directory is not set correctly
# see https://stackoverflow.com/questions/62987154/mysql-wont-start-error-su-warning-cannot-change-directory-to-nonexistent
sudo usermod -d /var/lib/mysql/ mysql

# Restart mysql to apply the changes
sudo service mysql restart
