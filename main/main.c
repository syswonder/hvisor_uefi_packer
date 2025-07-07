/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "acpi.h"
#include "arch.h"
#include "core.h"
#include "generated/autoconf.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
EFI_SYSTEM_TABLE *g_st;

// Helper function to print system information
static void print_system_info(EFI_SYSTEM_TABLE *SystemTable) {
  Print(L"[INFO] runtime services addr: 0x%lx\n", SystemTable->RuntimeServices);
  Print(L"[INFO] boot services addr: 0x%lx\n", SystemTable->BootServices);
  Print(L"[INFO] config table addr: 0x%lx\n", SystemTable->ConfigurationTable);
  Print(L"[INFO] con in addr: 0x%lx\n", SystemTable->ConIn);
  Print(L"[INFO] con out addr: 0x%lx\n", SystemTable->ConOut);
}

// Helper function to print binary information
static void print_binary_info(UINTN hvisor_bin_addr) {
  Print(L"---------------------------------------------------------------------"
        L"\n");
  Print(L"hvisor uefi packer target arch: %a\n", get_arch());
  Print(L"hvisor binary range: 0x%lx - 0x%lx\n", hvisor_bin_addr,
        hvisor_bin_addr + (hvisor_bin_end - hvisor_bin_start));
#if defined(CONFIG_ENABLE_VMLINUX)
  const UINTN hvisor_zone0_vmlinux_addr = CONFIG_VMLINUX_LOAD_ADDR;
  Print(L"hvisor vmlinux.bin: 0x%lx - 0x%lx\n", hvisor_zone0_vmlinux_addr,
        hvisor_zone0_vmlinux_addr +
            (hvisor_zone0_vmlinux_end - hvisor_zone0_vmlinux_start));
#endif
  Print(L"---------------------------------------------------------------------"
        L"\n");
}

// Helper function to copy hvisor binary
static void copy_hvisor_binary(UINTN hvisor_bin_addr) {
  memcpy2((void *)hvisor_bin_addr, (void *)hvisor_bin_start,
          hvisor_bin_end - hvisor_bin_start);
  Print(L"[INFO] hvisor binary copied to 0x%lx, size: 0x%lx\n", hvisor_bin_addr,
        hvisor_bin_end - hvisor_bin_start);
}

#if defined(CONFIG_ENABLE_VMLINUX)
// Helper function to copy vmlinux binary
static void copy_vmlinux_binary(void) {
  const UINTN hvisor_zone0_vmlinux_addr = CONFIG_VMLINUX_LOAD_ADDR;
  memcpy2((void *)hvisor_zone0_vmlinux_addr, (void *)hvisor_zone0_vmlinux_start,
          hvisor_zone0_vmlinux_end - hvisor_zone0_vmlinux_start);
  Print(L"[INFO] hvisor vmlinux.bin copied to 0x%lx, size: 0x%lx\n",
        hvisor_zone0_vmlinux_addr,
        hvisor_zone0_vmlinux_end - hvisor_zone0_vmlinux_start);
}
#endif

// Helper function to jump to hvisor
static void jump_to_hvisor(UINTN hvisor_bin_addr,
                           EFI_SYSTEM_TABLE *SystemTable) {
  UINTN system_table = (UINTN)SystemTable;
  void (*hvisor_entry)(UINTN, UINTN) = (void (*)(UINTN, UINTN))hvisor_bin_addr;

  print_str("[INFO] ok, ready to jump to hvisor entry...\n");
  hvisor_entry(0, system_table);

  // Should never reach here
  while (1) {
  }
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {

  EFI_STATUS status;

  // Initialize architecture abstraction layer
  arch_detect_and_init();

  print_str("\n\r");
  print_str("[INFO] arch_init done\n");

  InitializeLib(ImageHandle, SystemTable);
  Print(L"[INFO] UEFI bootloader initialized!\n");
  Print(L"[INFO] Hello! This is the UEFI bootloader of hvisor, arch = %a\n",
        get_arch());
  Print(L"[INFO] hvisor binary stored in .data, from 0x%lx to 0x%lx\n",
        hvisor_bin_start, hvisor_bin_end);
  Print(L"[INFO] CONFIG_EMBEDDED_HVISOR_BIN_PATH: %a\n",
        CONFIG_EMBEDDED_HVISOR_BIN_PATH);

  Print(L"[INFO] printing system info...\n");
  print_system_info(SystemTable);

  const UINTN hvisor_bin_addr = CONFIG_HVISOR_BIN_LOAD_ADDR;

  Print(L"[INFO] before exit boot services...\n");
  ARCH_BEFORE_EXIT_BOOT_SERVICES();

  Print(L"[INFO] printing binary info...\n");
  print_binary_info(hvisor_bin_addr);

  Print(L"[INFO] copying hvisor binary to 0x%lx...\n", hvisor_bin_addr);
  copy_hvisor_binary(hvisor_bin_addr);

#if defined(CONFIG_ENABLE_VMLINUX)
  Print(L"[INFO] copying vmlinux binary to 0x%lx...\n",
        CONFIG_VMLINUX_LOAD_ADDR);
  copy_vmlinux_binary();
#endif

  Print(L"[INFO] exiting boot services...\n");
  status = exit_boot_services(ImageHandle, SystemTable);
  print_str("[INFO] exit_boot_services done\n");

  // Serial is already initialized in arch_detect_and_init()
  jump_to_hvisor(hvisor_bin_addr, SystemTable);

  return EFI_SUCCESS;
}