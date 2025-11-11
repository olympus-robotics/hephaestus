#!/bin/bash

# Exit on error. Append "|| true" if you expect an error.
set -o errexit
# Exit on error inside any functions or subshells.
set -o errtrace
# Do not allow use of undefined vars. Use ${VAR:-} to use an undefined VAR
set -o nounset
# Catch the error in case mysqldump fails (but gzip succeeds) in `mysqldump | gzip`
set -o pipefail

# This script is used to configure a new virtual CAN interface used for debugging purposes,
# e.g. simulating a CAN network on the development machine.

if [ "$EUID" -ne 0 ]; then
  SUDO=sudo
else
  SUDO=""
fi

# Check if vcan0 interface already exists
if ! ip link show vcan0 >/dev/null 2>&1; then
  # The vcan0 interface does not exist, so we create it:

  # Create vcan0 interface
  ${SUDO} ip link add dev vcan0 type vcan

  # Set mtu to 72 in order to support CAN FD
  ${SUDO} ip link set vcan0 mtu 72

  # Bring up the vcan0 interface
  ${SUDO} ip link set up vcan0

  echo "Virtual CAN interface vcan0 has been set up."
else
  echo "The vcan0 interface already exists."
fi
