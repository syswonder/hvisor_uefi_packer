/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "arch.h"

#define UART_BASE 0x9000000
#define UART_FR 0x18          // Flag register
#define UART_FR_TXFF (1 << 5) // Transmit FIFO full
#define UART_DR 0x00          // Data register

static inline void mmio_write(uint64_t reg, uint32_t val) {
  *(volatile uint32_t *)(reg) = val;
}

static inline uint32_t mmio_read(uint64_t reg) {
  return *(volatile uint32_t *)(reg);
}

static void arch_serial_init(void) {}
static void arch_put_char(char c) {
  while (mmio_read(UART_BASE + UART_FR) & UART_FR_TXFF)
    ;
  mmio_write(UART_BASE + UART_DR, c);
}

static void arch_get_char(char *c) {}
static void arch_memory_init(void) {}
static void arch_setup_direct_mapping(void) {}
static void arch_clear_memory_regions(void) {}
static void arch_early_init(void) {}
static void arch_init(void) {}
static void arch_before_exit_boot_services(void) {}

static UINTN arch_get_boot_cpu_id(EFI_BOOT_SERVICES *g_bs) {
  // Implementation can be added if needed
  return 0;
}

struct arch_ops aarch64_ops = {
    .type = ARCH_AARCH64,
    .name = "aarch64",

    .early_init = arch_early_init,
    .init = arch_init,
    .before_exit_boot_services = arch_before_exit_boot_services,

    .get_boot_cpu_id = arch_get_boot_cpu_id,

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