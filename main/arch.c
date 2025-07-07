/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "arch.h"

#if defined(CONFIG_TARGET_ARCH_AARCH64)
extern struct arch_ops aarch64_ops;
#define ARCH_OPS &aarch64_ops
#elif defined(CONFIG_TARGET_ARCH_LOONGARCH64)
extern struct arch_ops loongarch64_ops;
#define ARCH_OPS &loongarch64_ops
#else
#error "Unsupported target architecture"
#endif

struct arch_ops *arch_ops = NULL;

void arch_detect_and_init(void) {
  arch_ops = ARCH_OPS;
  ARCH_EARLY_INIT();
  ARCH_INIT();
  ARCH_MEMORY_INIT();
  ARCH_SERIAL_INIT();
}