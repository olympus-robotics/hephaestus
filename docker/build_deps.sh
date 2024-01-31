#!/bin/bash

source ./version.sh

ncores=$(cat /proc/cpuinfo | grep processor | wc -l)
((ncores--))

SUFFIX=deps

pushd ../

BASE_IMAGE=ghcr.io/filippobrizzi/${IMAGE}:${VERSION}
docker pull ${BASE_IMAGE}
docker build . -t ${IMAGE}_${SUFFIX}:${VERSION} -f docker/Dockerfile_deps --cpuset-cpus "0-$ncores" --build-arg BASE_IMAGE=${BASE_IMAGE} --tag ${IMAGE}_${SUFFIX}:latest

popd
