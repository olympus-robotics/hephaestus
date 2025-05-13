#!/bin/bash

source ./version.sh

HOST_ARCH=$(uname -m)
ARCH=${1:-${HOST_ARCH}}
if [ "${ARCH}" == "aarch64" ]; then
    ARCH = "arm64"
fi

BASE_IMAGE=${HOST}/${ARCH}/${IMAGE}:${VERSION}
docker pull ${BASE_IMAGE}

SUFFIX=deps
IMAGE_NAME="${HOST}/${ARCH}/${IMAGE}_${SUFFIX}"

echo "Building image: ${IMAGE_NAME}"
pushd ../
docker build . -t ${IMAGE_NAME} -f docker/Dockerfile_deps --cpuset-cpus "0-$ncores" --build-arg BASE_IMAGE=${BASE_IMAGE} --tag ${IMAGE_NAME}:latest
popd

docker push ${IMAGE_NAME}
docker push ${IMAGE_NAME}:latest
