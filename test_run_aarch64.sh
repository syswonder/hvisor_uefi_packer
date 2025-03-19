# sudo apt-get install qemu-efi-aarch64
qemu-system-aarch64 -machine virt -cpu max -m 1024 -smp 1 \
  -bios ./firmware/QEMU_EFI_AARCH64.fd \
  -kernel BOOTAA64.EFI \
  -nographic
# no output... why?