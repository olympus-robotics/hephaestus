#!/bin/bash

source ./version.sh

ncores=$(cat /proc/cpuinfo | grep processor | wc -l)
((ncores--))

SUFFIX=deps

pushd ../

docker build . -t ${IMAGE}_${SUFFIX}:${VERSION} -f docker/Dockerfile_deps --cpuset-cpus "0-$ncores" --build-arg BASE_IMAGE=${IMAGE}:${VERSION} --build-arg CODEBASE=${CODEBASE} --progress=plain --no-cache

popd
