#include "generated/autoconf.h"

.section .hvisor.bin
.globl hvisor_bin_start, hvisor_bin_end
hvisor_bin_start:
.incbin CONFIG_EMBEDDED_HVISOR_BIN_PATH
hvisor_bin_end:

.section .hvisor.zone0_vmlinux
.globl hvisor_zone0_vmlinux_start, hvisor_zone0_vmlinux_end
hvisor_zone0_vmlinux_start:
.incbin CONFIG_EMBEDDED_VMLINUX_BIN_PATH
hvisor_zone0_vmlinux_end:
