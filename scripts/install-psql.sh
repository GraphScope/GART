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
sudo DEBIAN_FRONTEND=noninteractive apt-get -y install postgresql-$PSQL_VERSION

export PSQL_CONFIG=/etc/postgresql/$PSQL_VERSION/main/postgresql.conf

sudo tee -a $PSQL_CONFIG << EOT

wal_level = logical
max_replication_slots = 10  # larger than 0, default is 10
max_wal_senders = 10  # larger than 0, default is 10

EOT

# Install the development package for the PostgreSQL Extension:
sudo apt-get install -y postgresql-server-dev-$PSQL_VERSION

sudo /etc/init.d/postgresql restart

# pg_ctlcluster 16 main start
# use: sudo -u postgres -i psql
