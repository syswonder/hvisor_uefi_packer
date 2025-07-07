/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "arch.h"
#include "core.h"

extern void print_char(char c);
extern void init_serial(void);
extern void set_dmw(void);
extern void loongarch_arch_init(void);

static void arch_serial_init(void) { init_serial(); }
static void arch_put_char(char c) { print_char(c); }
static void arch_get_char(char *c) {}
static void arch_memory_init(void) {}
static void arch_setup_direct_mapping(void) {}
static void arch_clear_memory_regions(void) {
  // Don't clear the hvisor binary load address region (0x90000001f0000000)
  // as it contains the code we're about to jump to
  UINTN memset2_st = 0x9000000000000000ULL + 0x1000;
  UINTN memset2_size = 0x10000;
  UINTN memset3_st = 0x9000000000200000ULL;
  UINTN memset3_size = 0x1000000;
  
  // Only clear regions that don't overlap with the hvisor binary
  memset2((void *)memset2_st, 0, memset2_size);
  memset2((void *)memset3_st, 0, memset3_size);
}

static void arch_early_init(void) { set_dmw(); }
static void arch_init(void) { loongarch_arch_init(); }
static void arch_before_exit_boot_services(void) {
  arch_clear_memory_regions();
}

struct arch_ops loongarch64_ops = {
    .type = ARCH_LOONGARCH64,
    .name = "loongarch64",

    .early_init = arch_early_init,
    .init = arch_init,
    .before_exit_boot_services = arch_before_exit_boot_services,

    .serial =
        {
            .init = arch_serial_init,
            .put_char = arch_put_char,
            .get_char = arch_get_char,
        },

    .memory =
        {
            .init = arch_memory_init,
            .setup_direct_mapping = arch_setup_direct_mapping,
            .clear_memory_regions = arch_clear_memory_regions,
        },

    .arch_data = NULL,
};