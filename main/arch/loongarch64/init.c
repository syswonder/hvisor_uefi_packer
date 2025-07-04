#include "core.h"

void arch_before_exit_boot_services() {
    UINTN memset_st = 0x90000001f0000000ULL;
    UINTN memset_ed = 0x90000001f1000000ULL;
    UINTN memset2_st = 0x9000000000000000ULL + 0x1000;
    UINTN memset2_size = 0x10000;

    memset2((void *)memset_st, 0, memset_ed - memset_st);
    memset2((void *)memset2_st, 0, memset2_size);
}