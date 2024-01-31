#!/bin/bash

source ./version.sh

SUFFIX=deps

docker image tag ${IMAGE}_deps:${VERSION} ghcr.io/filippobrizzi/${IMAGE}_deps:${VERSION}
docker push ghcr.io/filippobrizzi/${IMAGE}_${SUFFIX}:${VERSION}
docker image tag ghcr.io/filippobrizzi/${IMAGE}_${SUFFIX}:${VERSION} ghcr.io/filippobrizzi/${IMAGE}_${SUFFIX}:latest
