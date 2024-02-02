#!/bin/bash

source ./version.sh

HOST_ARCH=$(uname -m)
ARCH=${1:-${HOST_ARCH}}
if [ "${ARCH}" == "aarch64" ]; then
    ARCH = "arm64"
fi

if [ "${ARCH}" == "x86_64" ]; then
    # see https://hub.docker.com/r/nvidia/opengl/tags
    BASE_IMAGE="nvidia/opengl:1.0-glvnd-runtime-ubuntu22.04"
elif [ "${ARCH}" == "arm64" ]; then
    BASE_IMAGE="arm64v8/ubuntu:22.04"
else
    echo "Supported arch value: arm64, x86_64"
    exit 1
fi

ncores=$(cat /proc/cpuinfo | grep processor | wc -l)

IMAGE_NAME="ghcr.io/filippobrizzi/${ARCH}/${IMAGE}"

function docker_tag_exists() {
    docker manifest inspect ${IMAGE_NAME}:${VERSION} > /dev/null
}

if docker_tag_exists; then
    echo "Image already exists, you can pull it with:"
    echo "$ docker pull ${IMAGE_NAME}:${VERSION}"
else
    echo "Building image: ${IMAGE_NAME}:${VERSION}"
    docker build -t ${IMAGE_NAME}:${VERSION} -f Dockerfile --cpuset-cpus "0-$ncores" --build-arg BASE_IMAGE=${BASE_IMAGE} . --tag ${IMAGE_NAME}:latest
    docker push ${IMAGE_NAME}:${VERSION}
    docker push ${IMAGE_NAME}:latest
fi
