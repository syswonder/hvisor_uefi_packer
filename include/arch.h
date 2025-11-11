/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <efi.h>
#include <efilib.h>

struct arch_ops;
struct arch_serial_ops;
struct arch_memory_ops;

typedef enum { ARCH_AARCH64, ARCH_LOONGARCH64, ARCH_RISCV64, ARCH_UNKNOWN } arch_type_t;

struct arch_serial_ops {
  void (*init)(void);
  void (*put_char)(char c);
  void (*get_char)(char *c);
};

struct arch_memory_ops {
  void (*init)(void);
  void (*setup_direct_mapping)(void);
  void (*clear_memory_regions)(void);
};

// Architecture operations structure
struct arch_ops {
  arch_type_t type;
  const char *name;

  void (*early_init)(void);
  void (*init)(void);
  UINTN (*get_boot_cpu_id)(EFI_BOOT_SERVICES *g_bs);
  void (*before_exit_boot_services)(void);

  struct arch_serial_ops serial;
  struct arch_memory_ops memory;

  void *arch_data;
};

extern struct arch_ops *arch_ops;

void arch_detect_and_init(void);

#define ARCH_TYPE() (arch_ops->type)
#define ARCH_NAME() (arch_ops->name)

#define ARCH_EARLY_INIT() arch_ops->early_init()
#define ARCH_INIT() arch_ops->init()
#define ARCH_BEFORE_EXIT_BOOT_SERVICES() arch_ops->before_exit_boot_services()

#define ARCH_GET_BOOT_CPU_ID(g_bs) arch_ops->get_boot_cpu_id(g_bs)

#define ARCH_SERIAL_INIT() arch_ops->serial.init()
#define ARCH_PUT_CHAR(c) arch_ops->serial.put_char(c)
#define ARCH_GET_CHAR(c) arch_ops->serial.get_char(c)

#define ARCH_MEMORY_INIT() arch_ops->memory.init()
#define ARCH_SETUP_DIRECT_MAPPING() arch_ops->memory.setup_direct_mapping()
#define ARCH_CLEAR_MEMORY_REGIONS() arch_ops->memory.clear_memory_regions()

#define ARCH_IS_AARCH64() (ARCH_TYPE() == ARCH_AARCH64)
#define ARCH_IS_LOONGARCH64() (ARCH_TYPE() == ARCH_LOONGARCH64)
#define ARCH_IS_RISCV64() (ARCH_TYPE() == ARCH_RISCV64)

static inline int arch_has_direct_mapping(void) {
  return ARCH_IS_LOONGARCH64();
}