#!/usr/bin/env bash
set -e
set -o pipefail

# Find all shell scripts in the current directory and its subdirectories
SHELLFILES=$(find ./ -type f -name "*.sh")
RESULT_CODE=""

# Iterate over each shellfile
for SHELLFILE in $SHELLFILES; do
  # Print the path of the shell file
  echo "-------- Linting $SHELLFILE via shellchecker ----------"

  set +e # Don't stop execution if shellcheck returns error code
  docker run --rm -i --name shellcheck -v "$(pwd):/mnt" koalaman/shellcheck:stable "$SHELLFILE" | tee shellcheck.report
  SHELL_RES=$?
  set -e
  cat shellcheck.report

  RESULT_CODE=$RESULT_CODE$SHELL_RES
  echo "ResultCode: $SHELL_RES"

  echo
done

echo "Summary of shellcheck: $RESULT_CODE"

exit $RESULT_CODE
