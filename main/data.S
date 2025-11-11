#include "generated/autoconf.h"

# .section .hvisor.bin
.globl hvisor_bin_start, hvisor_bin_end
hvisor_bin_start:
.incbin CONFIG_EMBEDDED_HVISOR_BIN_PATH
hvisor_bin_end:


# .section .hvisor.zone0_vmlinux
.globl hvisor_zone0_vmlinux_start, hvisor_zone0_vmlinux_end
hvisor_zone0_vmlinux_start:

#if defined(CONFIG_EMBEDDED_VMLINUX_PATH)
.incbin CONFIG_EMBEDDED_VMLINUX_PATH
#else
.byte 0, 0, 0, 0, 0, 0, 0, 0
#endif
hvisor_zone0_vmlinux_end:

