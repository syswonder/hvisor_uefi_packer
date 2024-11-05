CMD="make ARCH=loongarch64 V=1"
YELLOW_START="\033[1;33m"
YELLOW_END="\033[0m"
BOLD_START="\033[1m"
BOLD_END="\033[0m"
cd lib/gnu-efi && ./build.sh
cd ../..
echo -e "wheatfox(enkerewpo@hotmail.com) 2024"
echo -e "${BOLD_START}${YELLOW_START}Building hvisor UEFI Boot Image, CMD=$CMD${YELLOW_END}${BOLD_END}"
$CMD
echo -e "${BOLD_START}${YELLOW_START}Building hvisor shared object built, now building hvisor UEFI Boot Image${YELLOW_END}${BOLD_END}"
OBJCOPY="loongarch64-unknown-linux-gnu-objcopy"
CMD2="${OBJCOPY} -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* \
-j .rela.* -j .rel* -j .rela* -j .reloc -O binary hvisor-uefi-img hvisor-uefi-img.efi"
echo -e "${BOLD_START}${YELLOW_START}Building hvisor UEFI Boot Image, CMD=$CMD2${YELLOW_END}${BOLD_END}"
$CMD2
cp hvisor-uefi-img.efi BOOTLOONGARCH64.EFI
# print file info and size of BOOTLOONGARCH64.EFI
ls -lh BOOTLOONGARCH64.EFI
file BOOTLOONGARCH64.EFI
echo -e "${BOLD_START}${YELLOW_START}Building hvisor UEFI Boot Image done${YELLOW_END}${BOLD_END}"
