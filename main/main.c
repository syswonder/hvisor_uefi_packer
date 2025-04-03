// Copyright 2025 Syswonder
// SPDX-License-Identifier: MulanPSL-2.0

#include "acpi.h"
#include "core.h"
#include "parse.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
EFI_SYSTEM_TABLE *g_st;

UINTN memory_map_size = 0;
UINTN map_key, desc_size;
UINT32 desc_version;
EFI_MEMORY_DESCRIPTOR *memory_map_desc;

EFI_STATUS exit_boot_services(EFI_HANDLE ImageHandle,
                              EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS status;
  // get memory map
  memory_map_size = 0;
  memory_map_desc = NULL;
  status = uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5,
                             &memory_map_size, memory_map_desc, &map_key,
                             &desc_size, &desc_version);

  check(status, "GetMemoryMap (1st call)", EFI_BUFFER_TOO_SMALL, SystemTable);
  Print(L"[INFO] exit_boot_services: memory_map_size = %ld\n", memory_map_size);

  memory_map_size += 20 * desc_size;
  status = uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3,
                             EfiLoaderData, memory_map_size,
                             (void **)&memory_map_desc);
  if (memory_map_desc == NULL) {
    Print(L"[ERROR] exit_boot_services: AllocatePool failed !!!\n");
    halt();
  }

  // the below two call should be place just in next to each other
  // otherwise, the map key will changed when call ExitBootServices()!!
  status = uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5,
                             &memory_map_size, memory_map_desc, &map_key,
                             &desc_size, &desc_version);
  status = uefi_call_wrapper(SystemTable->BootServices->ExitBootServices, 2,
                             ImageHandle, map_key);
  return status;
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {

  EFI_STATUS status;

#if defined(CONFIG_TARGET_ARCH_LOONGARCH64)
  set_dmw(); // set up 0x8 and 0x9 DMW
#endif

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

  Print(L"[INFO] runtime services addr: 0x%lx\n", SystemTable->RuntimeServices);
  Print(L"[INFO] boot services addr: 0x%lx\n", SystemTable->BootServices);
  Print(L"[INFO] config table addr: 0x%lx\n", SystemTable->ConfigurationTable);
  Print(L"[INFO] con in addr: 0x%lx\n", SystemTable->ConIn);
  Print(L"[INFO] con out addr: 0x%lx\n", SystemTable->ConOut);

  // acpi_dump(SystemTable);

  // the entry is the same as the load address
  UINTN hvisor_bin_size = &hvisor_bin_end - &hvisor_bin_start;
  // UINTN hvisor_dtb_size = &hvisor_dtb_end - &hvisor_dtb_start;
  UINTN hvisor_zone0_vmlinux_size =
      &hvisor_zone0_vmlinux_end - &hvisor_zone0_vmlinux_start;

  UINTN hvisor_zone0_vmlinux_start_addr = (UINTN)&hvisor_zone0_vmlinux_start;
  // TODO: add to Kconfig or clean up code structure
  // this is a total mess ...
#if defined(CONFIG_TARGET_ARCH_LOONGARCH64)

  const UINTN hvisor_bin_addr = 0x9000000100010000ULL;
  const UINTN hvisor_zone0_vmlinux_addr = 0x9000000000200000ULL;
  const UINTN memset_st = 0x9000000100000000ULL;
  const UINTN memset_ed = 0x9000000101000000ULL;
  const UINTN memset2_st = 0x9000000000000000ULL + 0x1000;
  const UINTN memset2_size = 0x10000;

  // UEFI image parsing and loading
  const UINTN hvisor_zone0_vmlinux_efi_load_addr = 0x9000000120000000ULL;
  UINTN hvisor_zone0_vmlinux_efi_entry_addr;

#elif defined(CONFIG_TARGET_ARCH_AARCH64)

  const UINTN hvisor_bin_addr = 0x40400000;
  const UINTN hvisor_zone0_vmlinux_addr = 0xa0400000;

#else
#error "Unsupported target arch"
#endif

#if defined(CONFIG_TARGET_ARCH_LOONGARCH64)

  Print(L"[INFO] clearing memory from 0x%lx to 0x%lx\n", memset_st, memset_ed);
  memset2((void *)memset_st, 0, memset_ed - memset_st);
  // check memset
  for (UINTN i = memset_st; i < memset_ed; i++) {
    if (*(UINT8 *)i != 0) {
      Print(L"memset failed at 0x%lx !!!\n", i);
      halt();
    }
  }
  Print(L"[INFO] clearing memory from 0x%lx to 0x%lx\n", memset2_st,
        memset2_st + memset2_size);
  memset2((void *)memset2_st, 0, memset2_size);

  // 1. parse vmlinux.efi, get the offset of entry point
  // 2. load sections to hvisor_zone0_vmlinux_efi_load_addr (EFI image is PIC
  // code so we can load it to anywhere)
  // 3. later jump to entry

  // hvisor_zone0_vmlinux_efi_entry_addr = parse_pe(
  //     hvisor_zone0_vmlinux_start_addr, hvisor_zone0_vmlinux_efi_load_addr,
  //     hvisor_zone0_vmlinux_size); // parse vmlinux.efi and get entry addr
  // if (hvisor_zone0_vmlinux_efi_entry_addr == 0) {
  //   Print(L"[ERROR] parse_pe failed !!!\n");
  //   halt();
  // } else {
  //   Print(L"[INFO] parse_pe done, entry addr = 0x%lx\n",
  //         hvisor_zone0_vmlinux_efi_entry_addr);
  // }

#endif

  Print(L"====================================================================="
        L"==========\n");
  Print(L"hvisor uefi packer target arch:\t\t%a\n", get_arch());
  Print(L"hvisor binary range:\t\t\t0x%lx - 0x%lx\n", hvisor_bin_addr,
        hvisor_bin_addr + hvisor_bin_size);
  Print(L"hvisor vmlinux.bin :\t\t\t0x%lx - 0x%lx\n", hvisor_zone0_vmlinux_addr,
        hvisor_zone0_vmlinux_addr + hvisor_zone0_vmlinux_size);
  Print(L"====================================================================="
        L"==========\n");

  // dump first 8 instructions of hvisor binary
  // 0x12345677 0x00001234 ...
  for (int i = 0; i < 8; i++) {
    Print(L"0x%08x ", ((UINT32 *)hvisor_bin_start)[i]);
  }
  Print(L"\n");

  // now we have exited boot services
  memcpy2((void *)hvisor_bin_addr, (void *)hvisor_bin_start, hvisor_bin_size);
  Print(L"[INFO] hvisor binary copied\n");

  // memcpy2((void *)hvisor_dtb_addr, (void *)hvisor_dtb_start,
  // hvisor_dtb_size);
#ifndef CONFIG_TARGET_ARCH_AARCH64
  memcpy2((void *)hvisor_zone0_vmlinux_addr, (void *)hvisor_zone0_vmlinux_start,
          hvisor_zone0_vmlinux_size);
  Print(L"[INFO] hvisor vmlinux.bin copied\n");
#endif

  status = exit_boot_services(ImageHandle, SystemTable);
  print_str("[INFO] exit_boot_services done\n");

  init_serial();

#ifdef CONFIG_TARGET_ARCH_AARCH64
  // according to UEFI manual, aarch64 UEFI firmware will
  // use the highest non-secure EL to execute (non-secure EL1/EL2, but no EL3)
  __asm__ volatile("mrs x0, sctlr_el2\n"
                   "bic x0, x0, #1\n" // Clear M bit
                   "msr sctlr_el2, x0\n"
                   "isb\n");
#endif

  print_str("[INFO] ok, ready to jump to hvisor entry...\n");

  // void hvisor_entry(usize cpuid, usize dtb_addr/system_table, usize
  // entry_addr)

  UINTN system_table = (UINTN)SystemTable;

  void (*hvisor_entry)(UINTN, UINTN, UINTN) =
      (void (*)(UINTN, UINTN, UINTN))hvisor_bin_addr;

  // jump to hvisor entry
  hvisor_entry(0, system_table, 0);

  while (1) {
  }

  return EFI_SUCCESS;
}