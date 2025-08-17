FROM gcc:15 AS builder

RUN apt update && \
    apt install -y \
    cmake \
    git \
    libgflags-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    cmake --build . --config Release

FROM ubuntu:24.04

RUN apt update && \
    apt install -y \
    libgflags2.2 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/SimpleChessEngine /app/
WORKDIR /app

ENTRYPOINT ["./SimpleChessEngine"]

