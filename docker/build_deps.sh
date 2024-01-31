#!/bin/bash

source ./version.sh

HOST_ARCH=$(uname -m)
ARCH=${1:-${HOST_ARCH}}
if [ "${ARCH}" == "aarch64" ]; then
    ARCH = "arm64"
fi

BASE_IMAGE=ghcr.io/filippobrizzi/${ARCH}/${IMAGE}:${VERSION}
docker pull ${BASE_IMAGE}

SUFFIX=deps
IMAGE_NAME="ghcr.io/filippobrizzi/${ARC}/${IMAGE}_${SUFFIX}"

function docker_tag_exists() {
    docker manifest inspect ${IMAGE_NAME}:${VERSION} > /dev/null
}

if docker_tag_exists; then
    echo "Image already exists, pulling it: ${IMAGE_NAME}:${VERSION}"
    docker pull ${IMAGE_NAME}:${VERSION}
else
    echo "Building image: ${IMAGE_NAME}:${VERSION}"
    pushd ../
    docker build . -t ${IMAGE_NAME}:${VERSION} -f docker/Dockerfile_deps --cpuset-cpus "0-$ncores" --build-arg BASE_IMAGE=${BASE_IMAGE} --tag ${IMAGE_NAME}_${SUFFIX}:latest
    popd

    docker push ${IMAGE_NAME}:${VERSION}
    docker push ${IMAGE_NAME}:latest
fi
