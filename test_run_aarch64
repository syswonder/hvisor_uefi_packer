# sudo apt-get install qemu-efi-aarch64
echo "Running qemu-system-aarch64..."
qemu-system-aarch64 -machine virt,gic-version=3,virtualization=on,iommu=smmuv3 \
  -cpu max -m 2G -smp 1 \
  -bios ./firmware/QEMU_EFI_AARCH64.fd \
  -kernel BOOTAA64.EFI \
  -nographic \
  # -s -S