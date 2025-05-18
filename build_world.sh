RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_section() {
  echo -e "\n${YELLOW}=== $1 ===${NC}"
}

print_success() {
  echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
  echo -e "${RED}✗ $1${NC}"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ ! -f "$SCRIPT_DIR/.config" ]; then
  print_error ".config file not found in $SCRIPT_DIR"
  exit 1
fi

HVISOR_SRC_DIR=$(grep "^CONFIG_HVISOR_SRC_DIR=" "$SCRIPT_DIR/.config" | cut -d'"' -f2)
HVISOR_LA64_LINUX_DIR=$(grep "^CONFIG_HVISOR_LA64_LINUX_DIR=" "$SCRIPT_DIR/.config" | cut -d'"' -f2)
BUILDROOT_DIR=$(grep "^CONFIG_BUILDROOT_DIR=" "$SCRIPT_DIR/.config" | cut -d'"' -f2)
HVISOR_TOOL_DIR=$(grep "^CONFIG_HVISOR_TOOL_DIR=" "$SCRIPT_DIR/.config" | cut -d'"' -f2)

HVISOR_SRC_DIR=$(realpath "$HVISOR_SRC_DIR")
HVISOR_LA64_LINUX_DIR=$(realpath "$HVISOR_LA64_LINUX_DIR")
BUILDROOT_DIR=$(realpath "$BUILDROOT_DIR")
HVISOR_TOOL_DIR=$(realpath "$HVISOR_TOOL_DIR")

for dir in "$HVISOR_SRC_DIR" "$HVISOR_LA64_LINUX_DIR" "$BUILDROOT_DIR" "$HVISOR_TOOL_DIR"; do
  if [ ! -d "$dir" ]; then
    print_error "Directory not found: $dir"
    exit 1
  fi
done

HVISOR_LA64_LINUX_DIR_FULL_PATH="$HVISOR_LA64_LINUX_DIR"

if [ ! -f "$HVISOR_LA64_LINUX_DIR_FULL_PATH/chosen_root" ]; then
  print_error "CHOSEN_ROOT file not found at $HVISOR_LA64_LINUX_DIR_FULL_PATH/chosen_root"
  exit 1
fi

CHOSEN_ROOT=$(cat "$HVISOR_LA64_LINUX_DIR_FULL_PATH/chosen_root")
if [ -z "$CHOSEN_ROOT" ]; then
  print_error "CHOSEN_ROOT is not set in $HVISOR_LA64_LINUX_DIR_FULL_PATH/chosen_root"
  exit 1
fi

TOTAL_STEPS=6
CURRENT_STEP=0

unset LD_LIBRARY_PATH

print_section "Building World"

print_section "Building hvisor kernel module and cmd tool"
cd "$HVISOR_TOOL_DIR" && make ARCH=loongarch KDIR="$HVISOR_LA64_LINUX_DIR_FULL_PATH/linux-$CHOSEN_ROOT" all
if [ $? -ne 0 ]; then
  print_error "Failed to build hvisor kernel module and cmd tool"
  exit 1
fi
CURRENT_STEP=$((CURRENT_STEP + 1))

print_section "Copying hvisor kernel module and cmd tool"
cp "$HVISOR_TOOL_DIR/driver/hvisor.ko" "$BUILDROOT_DIR/board/loongson/ls3a5000/rootfs_ramdisk_overlay/tool/hvisor.ko"
cp "$HVISOR_TOOL_DIR/tools/hvisor" "$BUILDROOT_DIR/board/loongson/ls3a5000/rootfs_ramdisk_overlay/tool/hvisor"
CURRENT_STEP=$((CURRENT_STEP + 1))

print_section "Crafting nonroot rootfs cpio.gz"
cd $HVISOR_LA64_LINUX_DIR && ./build nonroot_setup
if [ $? -ne 0 ]; then
  print_error "Failed to craft nonroot rootfs cpio.gz"
  exit 1
fi
CURRENT_STEP=$((CURRENT_STEP + 1))

print_section "Building nonroot linux kernel"
cd "$HVISOR_LA64_LINUX_DIR" && ./build def nonroot && ./build kernel nonroot
if [ $? -ne 0 ]; then
  print_error "Failed to build nonroot linux kernel"
  exit 1
fi
CURRENT_STEP=$((CURRENT_STEP + 1))

print_section "Copying nonroot vmlinux.bin to buildroot"
cp "$HVISOR_LA64_LINUX_DIR/target/nonroot/vmlinux.bin" "$BUILDROOT_DIR/board/loongson/ls3a5000/rootfs_ramdisk_overlay/tool/vmlinux.bin"
# also copy the timestamp file
cp "$HVISOR_LA64_LINUX_DIR/target/nonroot/build_timestamp.txt" "$BUILDROOT_DIR/board/loongson/ls3a5000/rootfs_ramdisk_overlay/tool/nonroot_build_timestamp.txt"
CURRENT_STEP=$((CURRENT_STEP + 1))

print_section "Building buildroot"
cd "$BUILDROOT_DIR" && make -j12
if [ $? -ne 0 ]; then
  print_error "Failed to build buildroot"
  exit 1
fi
CURRENT_STEP=$((CURRENT_STEP + 1))

print_section "Building root linux kernel"
cd "$HVISOR_LA64_LINUX_DIR" && ./build def root && ./build kernel root
if [ $? -ne 0 ]; then
  print_error "Failed to build root linux kernel"
  exit 1
fi
CURRENT_STEP=$((CURRENT_STEP + 1))

print_success "Building world completed successfully!"
