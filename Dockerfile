FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    lsb-release \
    sudo \
    python3 \
    python3-pip \
    vim \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . /workspace/gart

WORKDIR /deps
RUN /workspace/gart/scripts/install-deps.sh
ENV KAFKA_HOME="/deps/kafka"
ENV MAXWELL_HOME="/deps/maxwell"

# Install PostgreSQL
# If you want to install MySQL, use install-mysql.sh instead of install-psql.sh
RUN /workspace/gart/scripts/install-psql.sh

RUN /workspace/gart/scripts/install-mysql.sh

WORKDIR /workspace
RUN git clone https://github.com/GraphScope/gstest.git

# clean up
RUN rm -rf /deps/cpprestsdk /deps/etcd-cpp-apiv3 /deps/libgrape-lite /deps/oneTBB /deps/pgql-lang /deps/v6d /deps/yaml-cpp

CMD ["/bin/bash"]

# Example:
# docker rm gart0; docker image rm gart; docker build -t gart .
# docker run -it --name gart0 gart
