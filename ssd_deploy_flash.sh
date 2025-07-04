#!/bin/bash

# Check if correct number of arguments are provided
if [ $# -ne 2 ]; then
    echo "Usage: $0 <rootfs.cpio.gz> <target_directory>"
    exit 1
fi

ROOTFS_ARCHIVE="$1"
TARGET_DIR="$2"


if [ ! -f "$ROOTFS_ARCHIVE" ]; then
    echo "Error: Rootfs archive '$ROOTFS_ARCHIVE' not found"
    exit 1
fi

if [ ! -d "$TARGET_DIR" ]; then
    echo "Creating target directory: $TARGET_DIR"
    mkdir -p "$TARGET_DIR"
fi

TEMP_DIR=$(mktemp -d)
echo "Created temporary directory: $TEMP_DIR"

echo "Extracting rootfs archive..."
gunzip -c "$ROOTFS_ARCHIVE" | (cd "$TEMP_DIR" && cpio -idm)

echo "Copying files to target directory..."
cp -a "$TEMP_DIR"/* "$TARGET_DIR"/

echo "Cleaning up..."
rm -rf "$TEMP_DIR"

echo "Deployment completed successfully!"
