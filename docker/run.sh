#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source "${DIR}/version.sh"

CONTAINER="hephaestus"
EXTRA_MOUNTS_=()
DEVICES_=()
NETWORKING=''
DOCKER_EXTRA_FLAGS_=()
ENTRYPOINT="/bin/bash"

ARCH=$(uname -m)
if [ "${ARCH}" == "aarch64" ]; then
    ARCH = "arm64"
fi
DEVIMAGE="${HOST}/${ARCH}/${IMAGE}:${VERSION}"

usage() {
    echo "usage: $0 ..."
    echo " "
    echo "  -n    Name of the image (default ${CONTAINER})."
    echo "  -i, --image"
    echo "        Specify the Docker image [default: ${DEVIMAGE}]"
    echo "  -v    The location of an additional directory to be mounted in the docker container."
    echo "        May be specified multiple times."
    echo "        NOTE: the directory is mounted in the same location of the host."
    echo "  -d, --device"
    echo "        path to additional devices to map."
    echo "  --entrypoint"
    echo "        Specify an alternative entry point [default: ${ENTRYPOINT}]"
}

while [ "$1" != "" ]; do
    case $1 in
        -n )
                          shift
                          CONTAINER=$1
                          ;;
        -v )              shift
                          EXTRA_MOUNTS_+=("$1")
                          ;;
        -i | --image)     shift
                          DEVIMAGE=_$1
                          ;;
        -d | --device )   shift
                          DEVICES_+=("$1")
                          ;;
        --entrypoint )    shift
                          ENTRYPOINT=$1
                          ;;
        -? | --help )     usage
                          exit
                          ;;
        * )               usage
                          exit
                          ;;
    esac
    shift
done



GPUOPT="--device=/dev/dri/card0:/dev/dri/card0"

if [ -f /usr/bin/nvidia-smi ]; then
    ## this is needed to support optimus - we can have nvidia drivers set up,
    ## but run an integrated GPU instead
    NVENV=$(glxinfo | grep "GL vendor string: NVIDIA")
    if [ "${NVENV}" != "" ]; then
        GPUOPT="--gpus all"
    fi
fi

EXTRA_MOUNTS=""
for m in "${EXTRA_MOUNTS_[@]}"; do
    if [ -d "${m}" ]; then
        EXTRA_MOUNTS="${EXTRA_MOUNTS} -v ${m}:${m}"
    else
        echo "Attempt to mount '${m}' but not a valid directory!"
        exit 1
    fi
done

DEVICES=""
for d in "${DEVICES_[@]}"; do
    if [ -e "${d}" ]; then
        DEVICES="${DEVICES} --device=${d}"
    else
        echo "Attempt to connect device '${d}' but not a valid device!"
        exit 1
    fi
done

docker run --privileged --shm-size=512m --cap-add=SYS_PTRACE --security-opt seccomp=unconfined $GPUOPT \
       --name ${CONTAINER} \
       -d \
       --network host  -ti \
       -v $XSOCK:$XSOCK \
       -v ${PWD}/..:/workdir/hephaestus \
       ${EXTRA_MOUNTS} \
       ${DEVICES} \
       ${DOCKER_EXTRA_FLAGS} \
       --entrypoint ${ENTRYPOINT}  \
