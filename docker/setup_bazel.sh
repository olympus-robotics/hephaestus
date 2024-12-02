#!/bin/bash

BAZELISK_VERSION="v1.20.0"
BAZELISK_REPO="https://github.com/bazelbuild/bazelisk/releases/download/"
BUILDTOOLS_VERSION="v7.1.2"
BUILDTOOLS_REPO="https://github.com/bazelbuild/buildtools/releases/download/"
BIN_PATH="/usr/local/bin"

# get arhitecture
[[ $(uname -m) == "arm64" ]] && ARCH="arm64" || ARCH="amd64"

# get os type
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="darwin"
else
    exit 1
fi

# download bazilisk and it's buildtools
wget -L ${BAZELISK_REPO}${BAZELISK_VERSION}/bazelisk-${OS}-${ARCH} -O $BIN_PATH/bazel
wget -L ${BUILDTOOLS_REPO}${BUILDTOOLS_VERSION}/buildifier-${OS}-${ARCH} -O $BIN_PATH/buildifier
wget -L ${BUILDTOOLS_REPO}${BUILDTOOLS_VERSION}/buildozer-${OS}-${ARCH} -O $BIN_PATH/buildozer
chmod +x $BIN_PATH/bazel $BIN_PATH/buildifier $BIN_PATH/buildozer
