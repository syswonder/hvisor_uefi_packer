/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "arch.h"
#include "efi.h"
#include "efilib.h"
#include "riscvbootptotocol.h"
#include "util.h"
/// edk2: UefiCpuPkg/Include/Protocol/RiscVBootProtocol.h

struct sbiret {
  long error;
  long value;
};

static inline struct sbiret sbi_ecall(unsigned long eid, unsigned long fid,
                                      unsigned long a0, unsigned long a1,
                                      unsigned long a2, unsigned long a3,
                                      unsigned long a4, unsigned long a5) {
  register unsigned long _a0 __asm__("a0") = a0;
  register unsigned long _a1 __asm__("a1") = a1;
  register unsigned long _a2 __asm__("a2") = a2;
  register unsigned long _a3 __asm__("a3") = a3;
  register unsigned long _a4 __asm__("a4") = a4;
  register unsigned long _a5 __asm__("a5") = a5;
  register unsigned long _a6 __asm__("a6") = fid;
  register unsigned long _a7 __asm__("a7") = eid;
  __asm__ volatile("ecall"
                   : "+r"(_a0), "+r"(_a1)
                   : "r"(_a2), "r"(_a3), "r"(_a4), "r"(_a5), "r"(_a6), "r"(_a7)
                   : "memory");
  struct sbiret ret = {.error = (long)_a0, .value = (long)_a1};
  return ret;
}

static void arch_serial_init(void) {}
static void arch_put_char(char c) {
  (void)sbi_ecall(0x1, 0, (unsigned long)c, 0, 0, 0, 0, 0);
}

static void arch_get_char(char *c) { UNUSED_ARG(c); }
static void arch_memory_init(void) {}
static void arch_setup_direct_mapping(void) {}
static void arch_clear_memory_regions(void) {}
static void arch_early_init(void) {}
static void arch_init(void) {
  // bad msg: edk2 has a mapping, we can only alloc use uefi interface.
  // good msg: edk2 mapping VA = PA, we disable mmu and flush tlb here.

  for(int i = 0; i < 256; i++)
    arch_put_char('a');
  // note: sfence.vma has another purpose: memory barrier
  __asm__ volatile("csrw satp, x0\n"
                   "sfence.vma x0, x0\n");
}
static void arch_before_exit_boot_services(void) {}

EFI_GUID gRiscVEfiBootProtocolGuid = {
    0xccd15fec,
    0x6f73,
    0x4eec,
    {0x83, 0x95, 0x3e, 0x69, 0xe4, 0xb9, 0x40, 0xbf}};

static UINTN arch_get_boot_hart_id() {
  EFI_STATUS Status;
  RISCV_EFI_BOOT_PROTOCOL *RiscvBootProtocol = NULL;

  Status = gBS->LocateProtocol(&gRiscVEfiBootProtocolGuid, NULL,
                               (VOID **)&RiscvBootProtocol);
  if (EFI_ERROR(Status)) {
    Print(L"Failed to locate RISCV_EFI_BOOT_PROTOCOL\n");
    return Status;
  }

  UINTN BootHartId;
  Status = RiscvBootProtocol->GetBootHartId(RiscvBootProtocol, &BootHartId);
  if (EFI_ERROR(Status)) {
    Print(L"Failed to get BootHartId\n");
    return Status;
  }

  Print(L"BootHartId: %lu\n", BootHartId);
  return BootHartId;
}

struct arch_ops riscv64_ops = {
    .type = ARCH_RISCV64,
    .name = "riscv64",

    .early_init = arch_early_init,
    .init = arch_init,
    .before_exit_boot_services = arch_before_exit_boot_services,

    .get_boot_cpu_id = arch_get_boot_hart_id,

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