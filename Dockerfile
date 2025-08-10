FROM gcc:15 AS builder

RUN apt update && \
    apt install -y \
    cmake \
    git \
    libgflags-dev \
    libgoogle-glog-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp
RUN git clone --branch v0.7.1 https://github.com/google/glog.git && \
    mkdir glog/build && \
    cd glog/build && \
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
    make -j$(nproc) && \
    make install

WORKDIR /app
COPY . .

RUN mkdir build && \
    cd build && \
    cmake .. && \
    cmake --build . --config Release

FROM ubuntu:24.04

RUN apt update && \
    apt install -y \
    libgflags2.2 \
    libgoogle-glog-dev \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/libglog.so* /usr/lib/
COPY --from=builder /app/build/SimpleChessEngine /app/
WORKDIR /app

ENTRYPOINT ["./SimpleChessEngine"]

