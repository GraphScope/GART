#!/bin/bash

# Reference: https://www.postgresql.org/download/linux/ubuntu/

# Create the file repository configuration:
sudo sh -c 'echo "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list'

# Import the repository signing key:
wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -

# Update the package lists:
sudo apt-get update

export PSQL_VERSION=16

# Install the specific version of PostgreSQL.
# If you want a latest version, use 'postgresql' instead of 'postgresql-$PSQL_VERSION':
sudo apt-get -y install postgresql-$PSQL_VERSION

export PSQL_CONFIG=/etc/postgresql/$PSQL_VERSION/main/postgresql.conf

sudo tee -a $PSQL_CONFIG << EOT

wal_level = logical
max_replication_slots = 1  # larger than 0
max_wal_senders = 1  # larger than 0

EOT

sudo /etc/init.d/postgresql restart

# pg_ctlcluster 12 main start
# use: sudo -u postgres -i psql
