#!/bin/bash

# Function to mount drive
mount_drive() {
    local drive="$1"
    local mount_point="$2"
    
    # Create mount point if it doesn't exist
    mkdir -p "$mount_point"
    
    # Try to mount the drive
    if mount "$drive" "$mount_point"; then
        echo "Successfully mounted $drive to $mount_point"
        return 0
    else
        echo "Failed to mount $drive"
        return 1
    fi
}

# Function to check partition format
check_partition_format() {
    local partition="$1"
    local expected_format="$2"
    local current_format=$(lsblk -no FSTYPE "$partition")
    
    if [ "$current_format" != "$expected_format" ]; then
        echo "Error: Partition $partition has incorrect format. Expected: $expected_format, Found: $current_format"
        return 1
    fi
    return 0
}

# Function to check if device exists and is accessible
check_device() {
    local device="$1"
    if [ ! -b "$device" ]; then
        echo "Error: Device $device does not exist or is not a block device"
        return 1
    fi
    return 0
}

# Function to cleanup mount points
cleanup() {
    echo "Cleaning up mount points..."
    for mount_point in "$PARTITION3_MOUNTPOINT" "$PARTITION4_MOUNTPOINT" "$USB_MOUNTPOINT"; do
        if mountpoint -q "$mount_point"; then
            umount "$mount_point" 2>/dev/null
        fi
        if [ -d "$mount_point" ]; then
            rmdir "$mount_point" 2>/dev/null
        fi
    done
}

# Set up cleanup on script exit and interrupt
trap cleanup EXIT INT TERM

# Create temporary mount points
PARTITION3_MOUNTPOINT="/tmp/hvisor_p3_$$"
PARTITION4_MOUNTPOINT="/tmp/hvisor_p4_$$"
USB_MOUNTPOINT="/tmp/hvisor_usb_$$"

# If no arguments provided, perform automatic mounting
if [ $# -eq 0 ]; then
    echo "No arguments provided. Performing automatic mounting..."
    
    # Create mount points
    mkdir -p "$PARTITION3_MOUNTPOINT" "$PARTITION4_MOUNTPOINT" "$USB_MOUNTPOINT" || {
        echo "Error: Failed to create mount points"
        exit 1
    }
    
    # Use /dev/sda1 as the USB drive
    selected_drive="/dev/sda1"
    echo "Using USB drive: $selected_drive"
    
    # Check if device exists
    if ! check_device "$selected_drive"; then
        echo "Error: USB drive not found"
        exit 1
    fi
    
    if ! mount_drive "$selected_drive" "$USB_MOUNTPOINT"; then
        echo "Error: Failed to mount USB drive"
        exit 1
    fi
    
    # Set deploy directory
    DEPLOY_DIR="$USB_MOUNTPOINT/deploy"
    
    if [ ! -d "$DEPLOY_DIR" ]; then
        echo "Error: deploy directory not found in USB drive"
        exit 1
    else
        echo "Deploy directory found in USB drive: contents: $(ls -la $DEPLOY_DIR)"
    fi
    
    # Mount partitions
    echo "Mounting partitions..."
    
    # Check partition formats before mounting
    echo "Checking partition formats..."
    if ! check_partition_format "/dev/nvme0n1p3" "vfat"; then
        echo "Error: Partition 3 must be FAT32 format"
        exit 1
    fi
    
    if ! check_partition_format "/dev/nvme0n1p4" "ext4"; then
        echo "Error: Partition 4 must be ext4 format"
        exit 1
    fi
    
    if ! mount_drive "/dev/nvme0n1p3" "$PARTITION3_MOUNTPOINT"; then
        echo "Error: Failed to mount partition 3"
        exit 1
    fi
    
    if ! mount_drive "/dev/nvme0n1p4" "$PARTITION4_MOUNTPOINT"; then
        echo "Error: Failed to mount partition 4"
        exit 1
    fi
else
    # Use provided arguments
    if [ $# -ne 3 ]; then
        echo "Usage: $0 [<deploy_dir> <partition3_mountpoint> <partition4_mountpoint>]"
        echo "If no arguments are provided, script will attempt automatic mounting"
        exit 1
    fi
    
    DEPLOY_DIR="$1"
    PARTITION3_MOUNTPOINT="$2"
    PARTITION4_MOUNTPOINT="$3"
fi

# Check if deploy directory exists
if [ ! -d "$DEPLOY_DIR" ]; then
    echo "Error: Deploy directory '$DEPLOY_DIR' does not exist"
    exit 1
fi

# Check if mount points exist
if [ ! -d "$PARTITION3_MOUNTPOINT" ]; then
    echo "Error: Partition 3 mount point '$PARTITION3_MOUNTPOINT' does not exist"
    exit 1
fi

if [ ! -d "$PARTITION4_MOUNTPOINT" ]; then
    echo "Error: Partition 4 mount point '$PARTITION4_MOUNTPOINT' does not exist"
    exit 1
fi

# Check if source files exist
if [ ! -f "$DEPLOY_DIR/HVISOR_UEFI.EFI" ]; then
    echo "Error: HVISOR_UEFI.EFI not found in '$DEPLOY_DIR'"
    exit 1
fi

if [ ! -d "$DEPLOY_DIR/DEPLOY_OVERLAY" ]; then
    echo "Error: DEPLOY_OVERLAY directory not found in '$DEPLOY_DIR'"
    exit 1
fi

# Copy HVISOR_UEFI.EFI to partition 3
echo "Copying HVISOR_UEFI.EFI to partition 3..."
cp -v "$DEPLOY_DIR/HVISOR_UEFI.EFI" "$PARTITION3_MOUNTPOINT/"

# Copy DEPLOY_OVERLAY to partition 4
echo "Copying DEPLOY_OVERLAY to partition 4..."
cp -rv "$DEPLOY_DIR/DEPLOY_OVERLAY"/* "$PARTITION4_MOUNTPOINT/"

echo "Deployment completed successfully!"
