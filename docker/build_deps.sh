#!/bin/bash

# shellcheck source=/dev/null
source ./version.sh

HOST_ARCH=$(uname -m)
ARCH=${1:-${HOST_ARCH}}
if [ "${ARCH}" == "aarch64" ]; then
    ARCH="arm64"
fi

BASE_IMAGE=${HOST}/${ARCH}/${IMAGE}:${VERSION}
docker pull "${BASE_IMAGE}"

SUFFIX=deps
IMAGE_NAME="${HOST}/${ARCH}/${IMAGE}_${SUFFIX}"

echo "Building image: ${IMAGE_NAME}"
pushd ../
# shellcheck disable=SC2154 # TODO Review and fix
docker build . -t "${IMAGE_NAME}" -f docker/Dockerfile_deps --cpuset-cpus "0-$ncores" --build-arg BASE_IMAGE="${BASE_IMAGE}" --tag "${IMAGE_NAME}:latest"
popd || exit

docker push "${IMAGE_NAME}"
docker push "${IMAGE_NAME}:latest"
