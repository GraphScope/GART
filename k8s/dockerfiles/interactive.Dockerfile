# Interactive engine

ARG REGISTRY=registry.cn-hongkong.aliyuncs.com
ARG BUILDER_VERSION=latest
ARG RUNTIME_VERSION=latest
FROM $REGISTRY/graphscope/graphscope-dev:$BUILDER_VERSION AS builder

ARG CI=false

ARG profile=release
ENV profile=$profile
RUN git clone https://github.com/alibaba/GraphScope.git /home/graphscope/GraphScope

RUN cd /home/graphscope/GraphScope/ && \
    if [ "${CI}" = "true" ]; then \
        cp -r artifacts/interactive /home/graphscope/install; \
    else \
        mkdir /home/graphscope/install; \
        . /home/graphscope/.graphscope_env; \
        make interactive-install BUILD_TYPE="$profile" INSTALL_PREFIX=/home/graphscope/install; \
    fi

############### RUNTIME: frontend #######################
# FROM ubuntu:22.04 AS frontend
FROM $REGISTRY/graphscope/vineyard-runtime:$RUNTIME_VERSION AS frontend

ENV DEBIAN_FRONTEND=noninteractive

ENV GRAPHSCOPE_HOME=/opt/graphscope
ENV PATH=$PATH:$GRAPHSCOPE_HOME/bin LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GRAPHSCOPE_HOME/lib

USER root

RUN apt-get update -y && \
    apt-get install -y sudo default-jdk tzdata python3-pip && \
    apt-get clean -y && \
    rm -rf /var/lib/apt/lists/*

RUN apt-get update -y && \
    apt-get install -y git build-essential cmake curl maven libssl-dev libclang-dev openmpi-bin libopenmpi-dev libprotobuf-dev protobuf-compiler-grpc && \
    apt-get clean -y && \
    rm -rf /var/lib/apt/lists/*

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH=/root/.cargo/bin:$PATH

RUN python3 -m pip install --no-cache-dir vineyard vineyard-io etcd3 --user

COPY --from=builder /home/graphscope/install/conf /opt/graphscope/conf

RUN mkdir -p /var/log/graphscope \
  && chown -R graphscope:graphscope /var/log/graphscope
RUN chmod a+wrx /tmp

WORKDIR /home/graphscope
RUN git clone https://github.com/doudoubobo/GraphScope.git -b v0.1.4 /home/graphscope/GraphScope
RUN cd /home/graphscope/GraphScope/interactive_engine/compiler && \
    make build

WORKDIR /home/graphscope
RUN git clone https://github.com/GraphScope/GART.git

############### RUNTIME: executor #######################
FROM $REGISTRY/graphscope/vineyard-runtime:$RUNTIME_VERSION AS executor

USER root

ENV RUST_BACKTRACE=1

RUN apt-get update -y && \
    apt-get install -y python3-pip default-jdk curl && \
    apt-get clean -y && \
    rm -rf /var/lib/apt/lists/*

RUN apt-get update -y && \
    apt-get install -y git build-essential cmake curl maven libssl-dev libclang-dev openmpi-bin libopenmpi-dev && \
    apt-get clean -y && \
    rm -rf /var/lib/apt/lists/*

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH=/root/.cargo/bin:$PATH

RUN arch=$(arch | sed s/aarch64/arm64/ | sed s/x86_64/amd64/) && \
    env arch=$arch curl -L -o /usr/bin/kubectl https://storage.googleapis.com/kubernetes-release/release/v1.19.2/bin/linux/$arch/kubectl
RUN chmod +x /usr/bin/kubectl

# vineyard.executor.properties, log configuration files
COPY --from=builder /home/graphscope/install/conf /opt/graphscope/conf

RUN chmod a+wrx /tmp /var/tmp

RUN python3 -m pip install --no-cache-dir vineyard vineyard-io flask --user



WORKDIR /home/graphscope
# Install dependencies for etcd3
RUN apt-get update && apt-get install -y \
    libgrpc-dev \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc \
    libboost-all-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/microsoft/cpprestsdk.git \
    && cd cpprestsdk \
    && mkdir -p build \
    && cd build \
    && cmake .. -DCPPREST_EXCLUDE_WEBSOCKETS=ON -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF \
    && make -j \
    && make install \
    && cd ../.. \
    && rm -rf cpprestsdk

RUN git clone https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3.git \
    && cd etcd-cpp-apiv3 \
    && mkdir -p build \
    && cd build \
    && cmake .. \
    && make -j \
    && make install \
    && cd ../.. \
    && rm -rf etcd-cpp-apiv3

RUN git clone https://github.com/GraphScope/GART.git \
    && cd GART \
    && git submodule update --init \
    && cd interfaces/grin \
    && mkdir -p build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j \
    && cp ./libgart_grin.so /usr/local/lib/ \
    && rm -rf /home/graphscope/GART

RUN git clone https://github.com/doudoubobo/GraphScope.git -b v0.1.4 /home/graphscope/GraphScope
RUN cd /home/graphscope/GraphScope/interactive_engine/executor/assembly/grin_gart && \
cargo build --release

WORKDIR /home/graphscope
RUN git clone https://github.com/GraphScope/GART.git

ENV PATH=${PATH}:/home/graphscope/.local/bin