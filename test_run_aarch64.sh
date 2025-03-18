# sudo apt-get install qemu-efi-aarch64
qemu-system-aarch64 -machine virt -smp 4 -m 4G -nographic \
  -bios firmware/QEMU_EFI_AARCH64.fd \
  -serial mon:stdio \
  -kernel ./BOOTAA64.EFI
# no output... why?