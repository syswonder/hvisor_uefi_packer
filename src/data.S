.section .hvisor.bin
.globl hvisor_bin_start, hvisor_bin_end
hvisor_bin_start:
.incbin "../../hvisor/target/loongarch64-unknown-none/debug/hvisor.bin"
hvisor_bin_end:

.section .hvisor.dtb
.globl hvisor_dtb_start, hvisor_dtb_end
hvisor_dtb_start:
.incbin "../../hvisor/images/loongarch64/devicetree/loongson3_ls7a.dtb"
hvisor_dtb_end:

.section .hvisor.zone0_vmlinux
.globl hvisor_zone0_vmlinux_start, hvisor_zone0_vmlinux_end
// this is vmlinux.bin. not the ELF vmlinux or EFI vmlinux.efi !!!
hvisor_zone0_vmlinux_start:
.incbin "../../linux-6.9.8-la64/linux-6.9.8/arch/loongarch/boot/vmlinux.bin"
hvisor_zone0_vmlinux_end: