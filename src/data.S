.section .hvisor.bin
.global hvisor_bin_start, hvisor_bin_end
hvisor_bin_start:
.incbin "../../hvisor/target/loongarch64-unknown-none/debug/hvisor.bin"
hvisor_bin_end:

.section .hvisor.dtb
.global hvisor_dtb_start, hvisor_dtb_end
hvisor_dtb_start:
.incbin "../../hvisor/images/loongarch64/devicetree/loongson3_ls7a.dtb"
hvisor_dtb_end: