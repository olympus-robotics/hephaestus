#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"

# Find all DOCKERFILES in the current directory and its subdirectories, ignoring '.dockerignore' files
DOCKERFILES=$(find ./ -type f -regex ".*[Dd]ockerfile.*" ! -name "*.[Dd]ockerignore")
result_code=""

# Iterate over each Dockerfile
for dockerfile in $DOCKERFILES; do
  # Print the path of the Dockerfile
  echo "-------- Linting $dockerfile via hadolint ----------"

  set +e # Don't stop execution if hadolint returns error code
  docker run --rm -i --name hado -v "$DIR/hadolint.yaml:/.config/hadolint.yaml" \
    hadolint/hadolint hadolint "$@" - <"$dockerfile" | tee hadolint.report
  hado_res=$?
  set -e

  # Get the return code of the docker https://stackoverflow.com/a/50153668/5006592
  # !! this must be executed directly after docker command

  cat hadolint.report

  result_code=$result_code$hado_res
  echo "ResultCode: $hado_res"

  echo
done

echo "Summary of hadolint: $result_code"

exit $result_code
