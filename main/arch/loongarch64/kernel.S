#include "loongarch.h"
#include "regdef.h"

.globl set_dmw
set_dmw:
    li.d    t0, CSR_DMW1_INIT
    csrwr   t0, LOONGARCH_CSR_DMWIN1
    ret

.globl loongarch_arch_init
loongarch_arch_init:
    csrwr   ra, LOONGARCH_CSR_KS0
    // set DMW in in mapping mode to avoid UEFI pagetable stuff :(
    // remember we are already in mapping mode!
    li.d    t0, CSR_DMW0_INIT
    csrwr   t0, LOONGARCH_CSR_DMWIN0 // 0x8 window uncached
    li.d    t0, CSR_DMW1_INIT
    csrwr   t0, LOONGARCH_CSR_DMWIN1 // 0x9 window cached
    csrrd   ra, LOONGARCH_CSR_KS0
    // now let's invalidate all TLBs
    invtlb  0, zero, zero
    ret
