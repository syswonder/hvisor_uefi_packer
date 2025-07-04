/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "acpi.h"
#include "core.h"
#include "generated/autoconf.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
EFI_SYSTEM_TABLE *g_st;

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {

  EFI_STATUS status;

  arch_init();

  print_str("\n\r");
  print_str("[INFO] arch_init done\n");

  InitializeLib(ImageHandle, SystemTable);
  Print(L"[INFO] UEFI bootloader initialized!\n");
  // Print(L"\n");
  Print(L"[INFO] Hello! This is the UEFI bootloader of hvisor, arch = %a\n",
        get_arch());
  Print(L"[INFO] hvisor binary stored in .data, from 0x%lx to 0x%lx\n",
        hvisor_bin_start, hvisor_bin_end);
  Print(L"[INFO] CONFIG_EMBEDDED_HVISOR_BIN_PATH: %a\n",
        CONFIG_EMBEDDED_HVISOR_BIN_PATH);

  Print(L"[INFO] runtime services addr: 0x%lx\n", SystemTable->RuntimeServices);
  Print(L"[INFO] boot services addr: 0x%lx\n", SystemTable->BootServices);
  Print(L"[INFO] config table addr: 0x%lx\n", SystemTable->ConfigurationTable);
  Print(L"[INFO] con in addr: 0x%lx\n", SystemTable->ConIn);
  Print(L"[INFO] con out addr: 0x%lx\n", SystemTable->ConOut);

  // acpi_dump(SystemTable);
  const UINTN hvisor_bin_addr = CONFIG_HVISOR_BIN_LOAD_ADDR;
#if defined(CONFIG_ENABLE_VMLINUX)
  const UINTN hvisor_zone0_vmlinux_addr = CONFIG_VMLINUX_LOAD_ADDR;
#endif

  arch_before_exit_boot_services();

  Print(L"---------------------------------------------------------------------\n");
  Print(L"hvisor uefi packer target arch: %a\n", get_arch());
  Print(L"hvisor binary range: 0x%lx - 0x%lx\n", hvisor_bin_addr,
        hvisor_bin_end);
#if defined(CONFIG_ENABLE_VMLINUX)
  Print(L"hvisor vmlinux.bin: 0x%lx - 0x%lx\n", hvisor_zone0_vmlinux_addr,
        hvisor_zone0_vmlinux_end);
#endif
  Print(L"---------------------------------------------------------------------\n");

  // now we have exited boot services
  memcpy2((void *)hvisor_bin_addr, (void *)hvisor_bin_start,
          hvisor_bin_end - hvisor_bin_start);
  Print(L"[INFO] hvisor binary copied\n");

#if defined(CONFIG_ENABLE_VMLINUX)
  memcpy2((void *)hvisor_zone0_vmlinux_addr, (void *)hvisor_zone0_vmlinux_start,
          hvisor_zone0_vmlinux_end - hvisor_zone0_vmlinux_start);
  Print(L"[INFO] hvisor vmlinux.bin copied\n");
#endif

  status = exit_boot_services(ImageHandle, SystemTable);
  print_str("[INFO] exit_boot_services done\n");

  init_serial();
  print_str("[INFO] ok, ready to jump to hvisor entry...\n");

  UINTN system_table = (UINTN)SystemTable;

  void (*hvisor_entry)(UINTN, UINTN) = (void (*)(UINTN, UINTN))hvisor_bin_addr;
  hvisor_entry(0, system_table);

  while (1) {
  }

  return EFI_SUCCESS;
}