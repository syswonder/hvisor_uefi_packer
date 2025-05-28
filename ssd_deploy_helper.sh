DEPLOY_OVERLAY_ROOT=DEPLOY_OVERLAY
TOOL_DIR=tool

rm -rf deploy
if [ ! -d "deploy" ]; then
    mkdir -p deploy
    mkdir -p deploy/$DEPLOY_OVERLAY_ROOT
    mkdir -p deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR
    mkdir -p deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/nonroot
    # mkdir -p deploy/EFI/BOOT
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

cp $HVISOR_UEFI_IMAGE deploy/HVISOR_UEFI.EFI

for nonroot_vmlinux in $HVISOR_LINUX_SRC/target/nonroot-linux*/vmlinux-linux*.bin; do
    zone_name=$(basename $(dirname $nonroot_vmlinux) | sed 's/nonroot-//')
    cp $nonroot_vmlinux deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/nonroot/vmlinux-$zone_name.bin
done

cp $rootfs_cpio_gz deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/rootfs.cpio.gz
dd if=/dev/zero of=deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/_.ext4 bs=1M count=256
if [ $? -ne 0 ]; then
    echo "Error: Failed to create ext4 image"
    exit 1
fi

sudo mkfs.ext4 deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/_.ext4
if [ $? -ne 0 ]; then
    echo "Error: Failed to format ext4 image"
    exit 1
fi

if [ ! -d "/mnt" ]; then
    sudo mkdir -p /mnt
fi

sudo mount -o loop deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/_.ext4 /mnt
if [ $? -ne 0 ]; then
    echo "Error: Failed to mount ext4 image"
    exit 1
fi

gunzip -c deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/rootfs.cpio.gz > deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/rootfs.cpio
if [ $? -ne 0 ]; then
    echo "Error: Failed to extract rootfs.cpio.gz"
    sudo umount /mnt
    exit 1
fi

WORK_DIR=$(pwd)
cd /mnt && sudo cpio -id < $WORK_DIR/deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/rootfs.cpio
if [ $? -ne 0 ]; then
    echo "Error: Failed to extract rootfs.cpio"
    cd - > /dev/null
    sudo umount /mnt
    exit 1
fi
cd - > /dev/null

sudo rm -rf /mnt/$TOOL_DIR
sudo rm -rf /mnt/*.sh
sudo rm -rf /mnt/nohup*
sudo rm -f /mnt/etc/hostname
sudo echo "nonroot-dedsec" | sudo tee /mnt/etc/hostname
sudo touch /mnt/this_is_virtio_blk_rootfs
# also sync etc/profile to nonroot zones
sudo cp $BUILDROOT_DIR/rootfs_ramdisk_overlay/etc/profile /mnt/etc/profile
sudo cp $BUILDROOT_DIR/rootfs_ramdisk_overlay/etc/inittab_nonroot /mnt/etc/inittab # use nonroot inittab with agetty on hvc0

sudo ls -l /mnt
sudo umount /mnt
if [ $? -ne 0 ]; then
    echo "Error: Failed to unmount /mnt"
    exit 1
fi

nonroot_zones=("linux1" "linux2" "linux3")
for zone_name in ${nonroot_zones[@]}; do
    cp deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/_.ext4 deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/$zone_name-disk.ext4
done

rm -rf deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/_.ext4
rm -rf deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/rootfs.cpio

for zone_name in ${nonroot_zones[@]}; do
    if [ -f "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/$zone_name.json" ]; then
        cp "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/$zone_name.json" "deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/$zone_name.json"
    else
        echo "Warning: $zone_name.json not found in $BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/"
    fi
done

# create deploy's etc dir if not exists
if [ ! -d "deploy/$DEPLOY_OVERLAY_ROOT/etc" ]; then
    mkdir -p deploy/$DEPLOY_OVERLAY_ROOT/etc
fi

cp $BUILDROOT_DIR/rootfs_ramdisk_overlay/etc/profile deploy/$DEPLOY_OVERLAY_ROOT/etc/profile # update our modified profile when deploy to root linux
cp $BUILDROOT_DIR/rootfs_ramdisk_overlay/etc/inittab deploy/$DEPLOY_OVERLAY_ROOT/etc/inittab # update our modified inittab when deploy to root linux

if [ -f "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/virtio_cfg.json" ]; then
    cp "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/virtio_cfg.json" "deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/virtio_cfg.json"
else
    echo "Warning: virtio_cfg.json not found in $BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/"
fi

if [ -f "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/test.bin" ]; then
    cp "$BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/test.bin" "deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/test.bin"
else
    echo "Warning: test.bin not found in $BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/"
fi

cp $BUILDROOT_DIR/rootfs_ramdisk_overlay/$TOOL_DIR/hvisor* deploy/$DEPLOY_OVERLAY_ROOT/$TOOL_DIR/
cp $BUILDROOT_DIR/rootfs_ramdisk_overlay/install.sh deploy/$DEPLOY_OVERLAY_ROOT/
cp $BUILDROOT_DIR/rootfs_ramdisk_overlay/daemon.sh deploy/$DEPLOY_OVERLAY_ROOT/
cp $BUILDROOT_DIR/rootfs_ramdisk_overlay/start.sh deploy/$DEPLOY_OVERLAY_ROOT/

cp ./ssd_deploy* deploy/maintenance/

echo "Deployment completed successfully!" 