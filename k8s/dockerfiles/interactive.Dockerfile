# Interactive engine

ARG REGISTRY=registry.cn-hongkong.aliyuncs.com
ARG BUILDER_VERSION=latest
ARG RUNTIME_VERSION=latest
FROM $REGISTRY/graphscope/graphscope-dev:$BUILDER_VERSION AS builder

ARG CI=false

ARG profile=release
ENV profile=$profile

############### RUNTIME: frontend #######################
# FROM ubuntu:22.04 AS frontend
FROM $REGISTRY/graphscope/vineyard-runtime:$RUNTIME_VERSION AS frontend

ENV DEBIAN_FRONTEND=noninteractive

ENV GRAPHSCOPE_HOME=/opt/graphscope
ENV PATH=$PATH:$GRAPHSCOPE_HOME/bin LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GRAPHSCOPE_HOME/lib

USER root

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    default-jdk tzdata python3-pip \
    git build-essential cmake curl libboost-all-dev \
    libssl-dev libgrpc-dev libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV PATH=/root/.cargo/bin:$PATH

RUN python3 -m pip install --no-cache-dir etcd3 --user

RUN git clone https://github.com/microsoft/cpprestsdk.git \
    && cd cpprestsdk \
    && mkdir -p build \
    && cd build \
    && cmake .. -DCPPREST_EXCLUDE_WEBSOCKETS=ON -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF \
    && make -j \
    && make install \
    && cd ../.. \
    && rm -rf cpprestsdk \
    && git clone https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3.git \
    && cd etcd-cpp-apiv3 \
    && mkdir -p build \
    && cd build \
    && cmake .. \
    && make -j \
    && make install \
    && cd ../.. \
    && rm -rf etcd-cpp-apiv3 \
    && git clone https://github.com/GraphScope/GART.git \
    && cd GART \
    && git submodule update --init \
    && cd interfaces/grin \
    && mkdir -p build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j \
    && cp ./libgart_grin.so /usr/local/lib/ \
    && rm -rf /home/graphscope/GART

WORKDIR /home/graphscope
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y \
    && apt-get update \
    && apt-get install -y --no-install-recommends maven \
    && git clone https://github.com/doudoubobo/GraphScope.git -b gart-gie-grin /home/graphscope/GraphScope \
    && cd /home/graphscope/GraphScope/interactive_engine/compiler \
    && make build \
    && apt-get remove -y maven \
    && apt-get clean  \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /home/graphscope/GraphScope/.git \
    && rm -rf /home/graphscope/GraphScope/docs \
    && rustup self uninstall -y \
    && rm -rf /root/.rustup \
    && rm -rf /root/.cargo \
    && rm -rf /root/.m2 \
    && rm -rf /usr/local/cargo/registry /usr/local/cargo/git \
    && rm -rf /home/graphscope/GraphScope/interactive_engine/executor/assembly/v6d/ \
    && rm -rf /home/graphscope/GraphScope/interactive_engine/executor/ir/target/release/deps 

WORKDIR /home/graphscope
RUN git clone https://github.com/GraphScope/GART.git

############### RUNTIME: executor #######################
FROM $REGISTRY/graphscope/vineyard-runtime:$RUNTIME_VERSION AS executor

USER root

ENV RUST_BACKTRACE=1

RUN apt-get update -y && \
    apt-get install -y python3-pip curl git \
    build-essential cmake libssl-dev \
    libgrpc-dev libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc libboost-all-dev && \
    apt-get clean -y && rm -rf /var/lib/apt/lists/*

ENV PATH=/root/.cargo/bin:$PATH

RUN python3 -m pip install --no-cache-dir vineyard vineyard-io flask --user

WORKDIR /home/graphscope

RUN git clone https://github.com/microsoft/cpprestsdk.git \
    && cd cpprestsdk \
    && mkdir -p build \
    && cd build \
    && cmake .. -DCPPREST_EXCLUDE_WEBSOCKETS=ON -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF \
    && make -j \
    && make install \
    && cd ../.. \
    && rm -rf cpprestsdk \
    && git clone https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3.git \
    && cd etcd-cpp-apiv3 \
    && mkdir -p build \
    && cd build \
    && cmake .. \
    && make -j \
    && make install \
    && cd ../.. \
    && rm -rf etcd-cpp-apiv3 \
    && git clone https://github.com/GraphScope/GART.git \
    && cd GART \
    && git submodule update --init \
    && cd interfaces/grin \
    && mkdir -p build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j \
    && cp ./libgart_grin.so /usr/local/lib/ \
    && rm -rf /home/graphscope/GART

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y \
    && git clone https://github.com/doudoubobo/GraphScope.git -b gart-gie-grin /home/graphscope/GraphScope \
    && cd /home/graphscope/GraphScope/interactive_engine/executor/assembly/grin_gart \
    && cargo build --release \
    && rm -rf /home/graphscope/GraphScope/.git \
    && rm -rf /home/graphscope/GraphScope/docs \
    && rm -rf /home/graphscope/GraphScope/interactive_engine/executor/assembly/grin_gart/target/release/deps \
    && rm -rf /home/graphscope/GraphScope/interactive_engine/executor/assembly/grin_gart/target/release/build \
    && rustup self uninstall -y \
    && rm -rf /root/.rustup \
    && rm -rf /root/.cargo \
    && rm -rf /root/.m2 \
    && rm -rf /usr/local/cargo/registry /usr/local/cargo/git

WORKDIR /home/graphscope
RUN git clone https://github.com/GraphScope/GART.git

ENV PATH=${PATH}:/home/graphscope/.local/bin