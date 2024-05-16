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

# Example:
# docker rm gart0; docker image rm gart
# docker build -t gart .
# docker run -it --name gart0 gart

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
  wget

RUN if [ "$build_type" = "All" ]; then \
  apt-get install -y openssh-server \
  && touch /workspace/env_script.sh \
  && mkdir -p /var/run/sshd \
  && echo "mkdir -p /root/.ssh && ssh-keygen -q -t rsa -N '' -f /root/.ssh/id_rsa" >> /workspace/env_script.sh \
  && echo "cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys" >> /workspace/env_script.sh \
  && echo "service ssh start" >> /workspace/env_script.sh; \
  fi

# Install PostgreSQL and MySQL
RUN if [ "$build_type" = "All" ]; then \
  /workspace/gart/scripts/install-psql.sh \
  && /workspace/gart/scripts/install-mysql.sh \
  && echo "service mysql start" >> /workspace/env_script.sh; \
  fi

WORKDIR /workspace
COPY . /workspace/gart

WORKDIR /deps
RUN /workspace/gart/scripts/install-deps.sh /deps $build_type

# Complete the installation
RUN rm -rf /var/lib/apt/lists/*

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
  else \
  echo 'Invalid build type specified'; exit 1; \
  fi \
  "

WORKDIR /workspace
RUN if [ "$build_type" = "All" ]; then \
  git clone https://github.com/GraphScope/gstest.git; \
  fi

CMD ["/bin/bash", "-c", ". /workspace/env_script.sh && /bin/bash"]
