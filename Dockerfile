# Copyright 2020-2023 Alibaba Group Holding Limited.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FROM ubuntu:22.04

# Define build type (All, Converter, Writer, Analyzer, Controller)
ARG build_type=All

RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    sudo \
    netcat-openbsd \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
RUN touch env_script.sh

RUN if [ "$build_type" = "All" ]; then \
  apt-get update && apt-get install -y --no-install-recommends \
  openssh-server \
  && rm -rf /var/lib/apt/lists/* \
  && mkdir -p /var/run/sshd \
  && echo 'mkdir -p /root/.ssh && ssh-keygen -q -t rsa -N "" -f /root/.ssh/id_rsa' >> /workspace/env_script.sh \
  && echo 'cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys' >> /workspace/env_script.sh \
  && echo 'service ssh start' >> /workspace/env_script.sh; \
  fi

WORKDIR /workspace
COPY . /workspace/gart
COPY ./scripts/kube_ssh /usr/local/bin/kube_ssh

# Install PostgreSQL and MySQL
RUN if [ "$build_type" = "All" ]; then \
  /workspace/gart/scripts/install-psql.sh \
  && /workspace/gart/scripts/install-mysql.sh \
  && echo "service mysql start" >> /workspace/env_script.sh; \
  fi

WORKDIR /deps
RUN if [ "$build_type" != "Controller" ]; then \
 /workspace/gart/scripts/install-deps.sh /deps $build_type; \
  fi

RUN if [ "$build_type" = "Controller" ]; then \
  curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl" && \
  chmod +x ./kubectl && \
  mv ./kubectl /usr/local/bin/kubectl && \
  apt-get update && apt-get install -y --no-install-recommends openmpi-bin libopenmpi-dev maven python3 python3-pip && \
  rm -rf /var/lib/apt/lists/* && \
  pip3 install --no-cache-dir tenacity==8.3.0 && \
  pip3 install --no-cache-dir flask kubernetes etcd3; \
  apt-get remove -y maven; \
  apt-get autoremove -y; \
  rm -rf /root/.m2; \
  git clone https://github.com/oracle/pgql-lang.git; \
  cd pgql-lang; \
  sh install.sh; \
  fi

# Find the Kafka directory and write its path to a file
RUN if [ "$build_type" = "All" ]; then \
  set -eux; \
  KAFKA_DIR=$(find /deps -maxdepth 1 -type d -name "kafka_*" -print -quit); \
  echo "export KAFKA_HOME=${KAFKA_DIR}" >> /etc/profile.d/env_path.sh; \
  echo "export MAXWELL_HOME=/deps/maxwell" >> /etc/profile.d/env_path.sh; \
  echo "source /etc/profile.d/env_path.sh" >> /workspace/env_script.sh; \
  fi

RUN chmod ugo+x /workspace/env_script.sh

ENV PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION="python"

RUN mkdir -p /workspace/gart/build && cd /workspace/gart/build
WORKDIR /workspace/gart/build

RUN bash -c "\
  if [ \"$build_type\" = 'All' ]; then \
  cmake .. -DADD_GAE_ENGINE=ON && make -j && sudo make install; \
  elif [ \"$build_type\" = 'Converter' ]; then \
  cmake .. -DADD_PGQL=OFF -DADD_VEGITO=OFF && make -j; \
  elif [ \"$build_type\" = 'Writer' ]; then \
  cmake .. -DADD_PGQL=OFF -DADD_CONVERTER=OFF && make -j; \
  elif [ \"$build_type\" = 'Analyzer' ]; then \
  cmake .. -DADD_PGQL=OFF -DADD_CONVERTER=OFF -DADD_VEGITO=OFF -DADD_GAE_ENGINE=ON && make -j; \
  else \
  echo 'Build as Controller'; \
  fi \
  "

WORKDIR /workspace
RUN if [ "$build_type" = "All" ]; then \
  git clone https://github.com/GraphScope/gstest.git; \
  fi

CMD ["/bin/bash", "-c", ". /workspace/env_script.sh && /bin/bash"]
