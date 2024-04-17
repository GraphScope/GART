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
    openssh-server \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /var/run/sshd

WORKDIR /workspace
COPY . /workspace/gart

WORKDIR /deps
RUN /workspace/gart/scripts/install-deps.sh
RUN rm -rf /var/lib/apt/lists/*
# Find the Kafka directory and write its path to a file
RUN set -eux; \
    KAFKA_DIR=$(find /deps -maxdepth 1 -type d -name "kafka_*" -print -quit); \
    echo "export KAFKA_HOME=${KAFKA_DIR}" >> /etc/profile.d/kafka_path.sh
RUN echo "source /etc/profile.d/kafka_path.sh" >> /workspace/env_script.sh
RUN echo "service ssh start" >> /workspace/env_script.sh
RUN chmod ugo+x /workspace/env_script.sh

ENV MAXWELL_HOME="/deps/maxwell"

ENV PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION="python"

# Install PostgreSQL
# If you want to install MySQL, use install-mysql.sh instead of install-psql.sh
RUN /workspace/gart/scripts/install-psql.sh

RUN /workspace/gart/scripts/install-mysql.sh

RUN echo "service mysql start" >> /workspace/env_script.sh

RUN echo "mkdir /root/.ssh && ssh-keygen -q -t rsa -N '' -f /root/.ssh/id_rsa" >> /workspace/env_script.sh
RUN echo "cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys" >> /workspace/env_script.sh

WORKDIR /workspace
RUN git clone https://github.com/GraphScope/gstest.git

# clean up
RUN rm -rf /deps/cpprestsdk /deps/etcd-cpp-apiv3 /deps/libgrape-lite /deps/oneTBB /deps/pgql-lang /deps/v6d /deps/yaml-cpp /deps/pybind11

CMD ["/bin/bash", "-c", ". /workspace/env_script.sh && /bin/bash"]

# Example:
# docker rm gart0; docker image rm gart; docker build -t gart .
# docker run -it --name gart0 gart
