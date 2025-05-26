# create dir deploy if not exists
TOOL_DIR=tool

rm -rf deploy
if [ ! -d "deploy" ]; then
    mkdir -p deploy
    mkdir -p deploy/$TOOL_DIR
    mkdir -p deploy/EFI/BOOT
    mkdir -p deploy/maintenance
fi

# parse .config to get important paths
# #
# Automatically generated file; DO NOT EDIT.
# hvisor UEFI Boot Image Packer Configuration Menu
#
# CONFIG_TARGET_ARCH_LOONGARCH64=y
# # CONFIG_TARGET_ARCH_AARCH64 is not set
# CONFIG_CROSS_COMPILE="loongarch64-unknown-linux-gnu-"
# CONFIG_EMBEDDED_HVISOR_BIN_PATH="../hvisor/target/loongarch64-unknown-none/debug/hvisor.bin"
# CONFIG_EMBEDDED_VMLINUX_PATH="../hvisor-la64-linux/target/root/vmlinux.bin"
# CONFIG_HVISOR_SRC_DIR="../hvisor"
# CONFIG_HVISOR_LA64_LINUX_DIR="../hvisor-la64-linux"
# CONFIG_BUILDROOT_DIR="../buildroot-loongarch64"
# CONFIG_HVISOR_TOOL_DIR="../hvisor-tool"
# CONFIG_HVISOR_LOG_LEVEL="info"

HVISOR_UEFI_IMAGE=./BOOTLOONGARCH64.EFI
HVISOR_SRC_DIR=$(grep "^CONFIG_HVISOR_SRC_DIR=" .config | cut -d'"' -f2)
HVISOR_LINUX_SRC=$(grep "^CONFIG_HVISOR_LA64_LINUX_DIR=" .config | cut -d'"' -f2)
BUILDROOT_DIR=$(grep "^CONFIG_BUILDROOT_DIR=" .config | cut -d'"' -f2)
HVISOR_TOOL_DIR=$(grep "^CONFIG_HVISOR_TOOL_DIR=" .config | cut -d'"' -f2)

# Debug output
echo "Parsed values from .config:"
echo "HVISOR_SRC_DIR: $HVISOR_SRC_DIR"
echo "HVISOR_LINUX_SRC: $HVISOR_LINUX_SRC"
echo "BUILDROOT_DIR: $BUILDROOT_DIR"
echo "HVISOR_TOOL_DIR: $HVISOR_TOOL_DIR"

# Check if .config exists
if [ ! -f ".config" ]; then
    echo "Error: .config file does not exist"
    exit 1
fi

# Verify required directories and files exist
if [ ! -d "$HVISOR_LINUX_SRC" ]; then
    echo "Error: HVISOR_LINUX_SRC directory ($HVISOR_LINUX_SRC) does not exist"
    exit 1
fi

if [ ! -d "$BUILDROOT_DIR" ]; then
    echo "Error: BUILDROOT_DIR ($BUILDROOT_DIR) does not exist"
    exit 1
fi

if [ ! -f "$HVISOR_UEFI_IMAGE" ]; then
    echo "Error: HVISOR_UEFI_IMAGE ($HVISOR_UEFI_IMAGE) does not exist"
    exit 1
fi

rootfs_cpio_gz=$BUILDROOT_DIR/output/images/rootfs.cpio.gz
echo "Looking for rootfs.cpio.gz at: $rootfs_cpio_gz"

if [ ! -f "$rootfs_cpio_gz" ]; then
    echo "Error: rootfs.cpio.gz ($rootfs_cpio_gz) does not exist"
    echo "Please ensure buildroot has been built and the file exists at the expected location"
    exit 1
fi

# 1. copy hvisor UEFI image to deploy/EFI/BOOT/BOOTLOONGARCH64.EFI
cp $HVISOR_UEFI_IMAGE deploy/EFI/BOOT/BOOTLOONGARCH64.EFI

# 2. copy nonroot vmlinux.bin to deploy/$TOOL_DIR
# iterate HVISOR_LINUX_SRC/target/nonroot-linux*/vmlinux-linux*.bin
# copy to deploy/$TOOL_DIR/vmlinux-{zone_name}.bin

for nonroot_vmlinux in $HVISOR_LINUX_SRC/target/nonroot-linux*/vmlinux-linux*.bin; do
    zone_name=$(basename $(dirname $nonroot_vmlinux) | sed 's/nonroot-//')
    cp $nonroot_vmlinux deploy/$TOOL_DIR/vmlinux-$zone_name.bin
done

# 3. then create virtio ext4 images for each nonroot zone (linux1-linux3) based on buildroot's output's ext4 image
# target rootfs.cpio.gz at BUILDROOT_DIR/output/images/rootfs.cpio.gz
# we create a canonical ext4 image for each nonroot zone from rootfs.cpio.gz
rootfs_cpio_gz=$BUILDROOT_DIR/output/images/rootfs.cpio.gz
cp $rootfs_cpio_gz deploy/$TOOL_DIR/rootfs.cpio.gz
# create an ext4 image with capacity 2GB
dd if=/dev/zero of=deploy/$TOOL_DIR/_.ext4 bs=1M count=2048
if [ $? -ne 0 ]; then
    echo "Error: Failed to create ext4 image"
    exit 1
fi

sudo mkfs.ext4 deploy/$TOOL_DIR/_.ext4
if [ $? -ne 0 ]; then
    echo "Error: Failed to format ext4 image"
    exit 1
fi

# Create mount point if it doesn't exist
if [ ! -d "/mnt" ]; then
    sudo mkdir -p /mnt
fi

sudo mount -o loop deploy/$TOOL_DIR/_.ext4 /mnt
if [ $? -ne 0 ]; then
    echo "Error: Failed to mount ext4 image"
    exit 1
fi

# unpack the contexts of rootfs.cpio.gz to the ext4 image
gunzip -c deploy/$TOOL_DIR/rootfs.cpio.gz > deploy/$TOOL_DIR/rootfs.cpio
if [ $? -ne 0 ]; then
    echo "Error: Failed to extract rootfs.cpio.gz"
    sudo umount /mnt
    exit 1
fi

WORK_DIR=$(pwd)
cd /mnt && sudo cpio -id < $WORK_DIR/deploy/$TOOL_DIR/rootfs.cpio
if [ $? -ne 0 ]; then
    echo "Error: Failed to extract rootfs.cpio"
    cd - > /dev/null
    sudo umount /mnt
    exit 1
fi
cd - > /dev/null

# remove /tool folder
sudo rm -rf /mnt/tool
sudo rm -rf /mnt/*.sh
sudo rm -rf /mnt/nohup*

sudo ls -l /mnt
sudo umount /mnt
if [ $? -ne 0 ]; then
    echo "Error: Failed to unmount /mnt"
    exit 1
fi

nonroot_zones=("linux1" "linux2" "linux3")
for zone_name in ${nonroot_zones[@]}; do
    cp deploy/$TOOL_DIR/_.ext4 deploy/$TOOL_DIR/$zone_name-disk.ext4
done

rm -rf deploy/$TOOL_DIR/_.ext4
rm -rf deploy/$TOOL_DIR/rootfs.cpio
rm -rf deploy/$TOOL_DIR/rootfs.cpio.gz

# now copy BUILDROOT_DIR/rootfs_ramdisk_overlay/tool/{zone_name}.json to deploy/$TOOL_DIR/{zone_name}.json
for zone_name in ${nonroot_zones[@]}; do
    if [ -f "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/$zone_name.json" ]; then
        cp "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/$zone_name.json" "deploy/$TOOL_DIR/$zone_name.json"
    else
        echo "Warning: $zone_name.json not found in $BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/"
    fi
done

# also copy virtio_cfg.json to deploy/$TOOL_DIR/virtio_cfg.json if it exists
if [ -f "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/virtio_cfg.json" ]; then
    cp "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/virtio_cfg.json" "deploy/$TOOL_DIR/virtio_cfg.json"
else
    echo "Warning: virtio_cfg.json not found in $BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/"
fi

# copy test.bin if it exists
if [ -f "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/test.bin" ]; then
    cp "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/test.bin" "deploy/$TOOL_DIR/test.bin"
else
    echo "Warning: test.bin not found in $BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/"
fi

cp $BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/hvisor* deploy/$TOOL_DIR/
cp $BUILDROOT_DIR/rootfs_ramdisk_overlay/hvisor-manager.sh deploy/

# copy ./ssd_deploy* to deploy/maintenance/
cp ./ssd_deploy* deploy/maintenance/

echo "Deployment completed successfully!" 