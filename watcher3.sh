#!/bin/bash

# 1. Setup Paths
OUTPUT_BASE=$(bazel info output_base)
WATCH_DIR="$OUTPUT_BASE"
DEST_DIR="/workspaces/jsontraces"

rm -Rf "$DEST_DIR"
mkdir -p "$DEST_DIR"

echo "---------------------------------------------------"
echo "Polling Directory: $WATCH_DIR"
echo "Target Directory:  $DEST_DIR"
echo "---------------------------------------------------"

# 2. Wait for Bazel to create the sandbox
echo "Waiting for sandbox directory to appear..."
while [ ! -d "$WATCH_DIR" ]; do
    sleep 1
done

echo "✅ Sandbox detected. Polling for .json files..."

# 3. Infinite Polling Loop
while true; do
    # Find all .json files in the sandbox
    # We suppress errors (2>/dev/null) in case Bazel deletes a dir while find is scanning
    find "$WATCH_DIR" -name "*.json" 2>/dev/null | while read -r filepath; do
        
        # Create a unique filename for the destination
        # We replace slashes with underscores to flatten the directory structure
        # Remove the common prefix to keep names reasonable
        flat_name=$(echo "$filepath" | sed "s|$WATCH_DIR/||" | sed 's|/|_|g')
        dest_path="$DEST_DIR/$flat_name"

        # Only copy if we haven't copied it yet
        if [ ! -f "$dest_path" ]; then
            cp "$filepath" "$dest_path"
            echo "Captured: $flat_name"
        fi
    done

    # Wait 1 second before next scan
    sleep 1
    echo "Waiting..."
done
