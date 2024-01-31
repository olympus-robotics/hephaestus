#!/bin/bash

source ./version.sh


ARCH=$(uname -m)

if [ "${ARCH}" == "x86_64" ]; then
    # see https://hub.docker.com/r/nvidia/opengl/tags
    BASE_IMAGE="nvidia/opengl:1.0-glvnd-runtime-ubuntu22.04"
else
    BASE_IMAGE="arm64v8/ubuntu:22.04"
fi

ncores=$(cat /proc/cpuinfo | grep processor | wc -l)

docker build -t ${IMAGE}:${VERSION} -f Dockerfile --cpuset-cpus "0-$ncores" --build-arg BASE_IMAGE=${BASE_IMAGE} . --tag ${IMAGE}:latest
