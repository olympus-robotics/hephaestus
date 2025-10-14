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

# Use docker to check 
if $write_mode; then
   docker run --rm -t -v "$(pwd):/app" imunew/typos-cli  -w /app
else
   docker run --rm -t -v "$(pwd):/app" imunew/typos-cli /app
fi
spell_res=$?
exit $spell_res
