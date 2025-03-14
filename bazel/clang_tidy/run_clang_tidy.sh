#!/usr/bin/env bash
# Usage: run_clang_tidy <OUTPUT> <CONFIG> [ARGS...]

# =================================================================================================
# Copyright (C) 2020 Benedek Thaler Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

set -ue

CLANG_TIDY_BIN=$1
shift

OUTPUT=$1
shift

CONFIG=$1
shift

# clang-tidy doesn't create a patchfile if there are no errors.
# make sure the output exists, and empty if there are no errors,
# so the build system will not be confused.
touch $OUTPUT
truncate -s 0 $OUTPUT

# if $CONFIG is provided by some external workspace, we need to
# place it in the current directory
test -e .clang-tidy || ln -s -f $CONFIG .clang-tidy

# Print output on failure only
logfile="$(mktemp)"
trap 'if (($?)); then cat "$logfile" 1>&2; fi; rm "$logfile"' EXIT
# trap 'head -n 15 "$logfile" 1>&2' EXIT

"${CLANG_TIDY_BIN}" "$@" >"$logfile" 2>&1
