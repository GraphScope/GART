FROM ubuntu:22.04

# Define build type
ARG build_type=All

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

RUN if [ "$build_type" = "All" ]; then \
    apt-get update && apt-get install -y \
    default-jdk \
    openssh-server \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /var/run/sshd; \
fi

WORKDIR /workspace
COPY . /workspace/gart

WORKDIR /deps
RUN /workspace/gart/scripts/install-deps.sh /deps
RUN rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
RUN touch env_script.sh

# Find the Kafka directory and write its path to a file
RUN if [ "$build_type" = "All" ]; then \
    set -eux; \
    KAFKA_DIR=$(find /deps -maxdepth 1 -type d -name "kafka_*" -print -quit); \
    echo "export KAFKA_HOME=${KAFKA_DIR}" >> /etc/profile.d/env_path.sh; \
    echo "export MAXWELL_HOME=/deps/maxwell" >> /etc/profile.d/env_path.sh; \
    echo "source /etc/profile.d/env_path.sh" >> /workspace/env_script.sh; \
    chmod ugo+x /workspace/env_script.sh; \
fi

ENV PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION="python"

# Install PostgreSQL
# If you want to install MySQL, use install-mysql.sh instead of install-psql.sh
RUN if [ "$build_type" = "All" ]; then \
    /workspace/gart/scripts/install-psql.sh \
    && /workspace/gart/scripts/install-mysql.sh \
    && echo "service mysql start" >> /workspace/env_script.sh \
    && echo "mkdir -p /root/.ssh && ssh-keygen -q -t rsa -N '' -f /root/.ssh/id_rsa" >> /workspace/env_script.sh \
    && echo "cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys" >> /workspace/env_script.sh \
    && echo "service ssh start" >> /workspace/env_script.sh; \
fi


RUN mkdir -p /workspace/gart/build && cd /workspace/gart/build && cmake .. -DADD_GAE_ENGINE=ON && make -j && sudo make install

WORKDIR /workspace
RUN if [ "$build_type" = "All" ]; then \
    git clone https://github.com/GraphScope/gstest.git; \
fi 

# clean up
RUN rm -rf /deps/cpprestsdk /deps/etcd-cpp-apiv3 /deps/libgrape-lite /deps/oneTBB /deps/pgql-lang /deps/v6d /deps/yaml-cpp /deps/pybind11

CMD ["/bin/bash", "-c", ". /workspace/env_script.sh && /bin/bash"]

# Example:
# docker rm gart0; docker image rm gart; docker build -t gart .
# docker run -it --name gart0 gart
