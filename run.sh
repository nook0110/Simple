#!/bin/bash
IMAGE="sce-engine:latest"
docker build -t $IMAGE . | tee /dev/tty
docker run --rm -it --ulimit stack=-1:-1 $IMAGE

