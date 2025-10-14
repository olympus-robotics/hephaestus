#!/bin/bash 

write_mode=false

# Parse options
while getopts "w" opt; do
  case $opt in
    w)
      write_mode=true
      ;;
    *)
      echo "Usage: $0 [-w]   # -w: write output to file"
      exit 1
      ;;
  esac
done

# shellcheck disable=SC2329 # This is used in TRAP
function cleanup()
{
    rm -rf "${tmp_typo}"
}

trap cleanup EXIT

# Create tmp dir for binary
tmp_typo="$(mktemp -d)"
TYPOS_VERSION="$(curl -s "https://api.github.com/repos/crate-ci/typos/releases/latest" | grep -Po '"tag_name": "v\K[0-9.]+')"
wget -qO /tmp/typos.tar.gz "https://github.com/crate-ci/typos/releases/latest/download/typos-v$TYPOS_VERSION-x86_64-unknown-linux-musl.tar.gz"
tar xf /tmp/typos.tar.gz -C "$tmp_typo" ./typos
cd "$(git rev-parse --show-toplevel)" || exit
if $write_mode; then
   "${tmp_typo}/typos" -w
else
   "${tmp_typo}/typos"
fi
spell_res=$?
exit $spell_res
