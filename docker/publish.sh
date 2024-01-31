#!/bin/bash

source ./version.sh

docker image tag ${IMAGE}:${VERSION} ghcr.io/filippobrizzi/${IMAGE}:${VERSION}
docker push ghcr.io/filippobrizzi/${IMAGE}:${VERSION}
docker image tag ghcr.io/filippobrizzi/${IMAGE}:${VERSION} ghcr.io/filippobrizzi/${IMAGE}:latest
