#include "core.h"
#include <efi.h>
#include <efilib.h>

EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
EFI_SYSTEM_TABLE *g_st;

UINTN memory_map_size = 0;
UINTN map_key, desc_size;
UINT32 desc_version;
EFI_MEMORY_DESCRIPTOR *memory_map_desc;

void memcpy2(void *dest, void *src, int n) {
  char *csrc = (char *)src;
  char *cdest = (char *)dest;

  for (int i = 0; i < n; i++) {
    cdest[i] = csrc[i];
  }
}

void memset2(void *dest, int val, int n) {
  char *cdest = (char *)dest;

  for (int i = 0; i < n; i++) {
    cdest[i] = val;
  }
}

void halt() {
  Print(L"[INFO] (halt) loop forever\n");
  while (1)
    ;
}

void print_char(char c);
void print_str(const char *str) {
  while (*str) {
    print_char(*str);
    str++;
  }
}

char *get_efi_status_string(EFI_STATUS status) {
  switch (status) {
  case EFI_SUCCESS:
    return "EFI_SUCCESS";
  case EFI_LOAD_ERROR:
    return "EFI_LOAD_ERROR";
  case EFI_INVALID_PARAMETER:
    return "EFI_INVALID_PARAMETER";
  case EFI_UNSUPPORTED:
    return "EFI_UNSUPPORTED";
  case EFI_BAD_BUFFER_SIZE:
    return "EFI_BAD_BUFFER_SIZE";
  case EFI_BUFFER_TOO_SMALL:
    return "EFI_BUFFER_TOO_SMALL";
  case EFI_NOT_READY:
    return "EFI_NOT_READY";
  case EFI_DEVICE_ERROR:
    return "EFI_DEVICE_ERROR";
  case EFI_WRITE_PROTECTED:
    return "EFI_WRITE_PROTECTED";
  case EFI_OUT_OF_RESOURCES:
    return "EFI_OUT_OF_RESOURCES";
  case EFI_VOLUME_CORRUPTED:
    return "EFI_VOLUME_CORRUPTED";
  case EFI_VOLUME_FULL:
    return "EFI_VOLUME_FULL";
  case EFI_NO_MEDIA:
    return "EFI_NO_MEDIA";
  case EFI_MEDIA_CHANGED:
    return "EFI_MEDIA_CHANGED";
  case EFI_NOT_FOUND:
    return "EFI_NOT_FOUND";
  case EFI_ACCESS_DENIED:
    return "EFI_ACCESS_DENIED";
  case EFI_NO_RESPONSE:
    return "EFI_NO_RESPONSE";
  case EFI_NO_MAPPING:
    return "EFI_NO_MAPPING";
  case EFI_TIMEOUT:
    return "EFI_TIMEOUT";
  case EFI_NOT_STARTED:
    return "EFI_NOT_STARTED";
  case EFI_ALREADY_STARTED:
    return "EFI_ALREADY_STARTED";
  case EFI_ABORTED:
    return "EFI_ABORTED";
  case EFI_ICMP_ERROR:
    return "EFI_ICMP_ERROR";
  case EFI_TFTP_ERROR:
    return "EFI_TFTP_ERROR";
  case EFI_PROTOCOL_ERROR:
    return "EFI_PROTOCOL_ERROR";
  case EFI_INCOMPATIBLE_VERSION:
    return "EFI_INCOMPATIBLE_VERSION";
  case EFI_SECURITY_VIOLATION:
    return "EFI_SECURITY_VIOLATION";
  case EFI_CRC_ERROR:
    return "EFI_CRC_ERROR";
  case EFI_END_OF_MEDIA:
    return "EFI_END_OF_MEDIA";
  case EFI_END_OF_FILE:
    return "EFI_END_OF_FILE";
  case EFI_INVALID_LANGUAGE:
    return "EFI_INVALID_LANGUAGE";
  case EFI_COMPROMISED_DATA:
    return "EFI_COMPROMISED_DATA";
  default: {
    // print code in string
    static char buf[17];
    buf[16] = '\0';
    for (int i = 0; i < 16; i++) {
      buf[15 - i] = "0123456789ABCDEF"[(status >> (i * 4)) & 0xF];
      return buf;
    }
  }
  }
  return "UNKNOWN";
}

void check(EFI_STATUS status, const char *prefix, EFI_STATUS expected,
           EFI_SYSTEM_TABLE *SystemTable) {
  if (status != expected) {
    uefi_call_wrapper(SystemTable->ConOut->SetAttribute, 2, SystemTable->ConOut,
                      EFI_LIGHTRED);
    Print(L"[ERROR] (check) %a failed, should be %a, but got %a\n", prefix,
          get_efi_status_string(expected), get_efi_status_string(status));
    uefi_call_wrapper(SystemTable->ConOut->SetAttribute, 2, SystemTable->ConOut,
                      EFI_LIGHTGRAY);
    halt();
  }
  Print(L"[INFO] (check) %a success\n", prefix);
}

EFI_STATUS GetMemoryMapAndExitBootServices(EFI_HANDLE ImageHandle,
                                           EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS status;
  // get memory map
  memory_map_size = 0;
  memory_map_desc = NULL;
  status = uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5,
                             &memory_map_size, memory_map_desc, &map_key,
                             &desc_size, &desc_version);

  check(status, "GetMemoryMap (1st call)", EFI_BUFFER_TOO_SMALL, SystemTable);
  Print(L"[INFO] (GetMemoryMapAndExitBootServices) memory_map_size = %ld\n",
        memory_map_size);

  memory_map_size += 20 * desc_size;
  status = uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3,
                             EfiLoaderData, memory_map_size,
                             (void **)&memory_map_desc);
  if (memory_map_desc == NULL) {
    Print(L"[ERROR] (GetMemoryMapAndExitBootServices) AllocatePool failed\n");
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
  InitializeLib(ImageHandle, SystemTable);
  Print(L"\n");
  Print(L"Hello! This is the UEFI bootloader of hvisor(loongarch)...\n");
  Print(L"hvisor binary stored in .data, from 0x%lx to 0x%lx\n",
        hvisor_bin_start, hvisor_bin_end);
  Print(L"hvisor dtb stored in .data, from 0x%lx to 0x%lx\n", hvisor_dtb_start,
        hvisor_dtb_end);

  // set up 0x8 and 0x9 DMW
  set_dmw();
  arch_init();

  // the entry is the same as the load address

  UINTN hvisor_bin_size = &hvisor_bin_end - &hvisor_bin_start;
  UINTN hvisor_dtb_size = &hvisor_dtb_end - &hvisor_dtb_start;
  UINTN hvisor_zone0_vmlinux_size =
      &hvisor_zone0_vmlinux_end - &hvisor_zone0_vmlinux_start;

  const UINTN hvisor_bin_addr = 0x9000000100010000ULL;
  const UINTN hvisor_dtb_addr = 0x900000010000f000ULL;
  const UINTN hvisor_zone0_vmlinux_addr =
      0x9000000000200000ULL; // caution: this is actually a vmlinux.bin, not a
                             // vmlinux - wheatfox

  const UINTN memset_st = 0x9000000100000000ULL;
  const UINTN memset_ed = 0x9000000101000000ULL;

  Print(L"Clearing memory from 0x%lx to 0x%lx\n", memset_st, memset_ed);
  memset2((void *)memset_st, 0, memset_ed - memset_st);
  // check memset
  for (UINTN i = memset_st; i < memset_ed; i++) {
    if (*(UINT8 *)i != 0) {
      Print(L"memset failed at 0x%lx\n", i);
      halt();
    }
  }

  // print range overview
  Print(L"====================================================================="
        L"===========\n");
  Print(L"hvisor binary range:\t\t\t0x%lx - 0x%lx\n", hvisor_bin_addr,
        hvisor_bin_addr + hvisor_bin_size);
  Print(L"hvisor dtb range:\t\t\t0x%lx - 0x%lx\n", hvisor_dtb_addr,
        hvisor_dtb_addr + hvisor_dtb_size);
  Print(L"hvisor vmlinux.bin :\t\t\t0x%lx - 0x%lx\n", hvisor_zone0_vmlinux_addr,
        hvisor_zone0_vmlinux_addr + hvisor_zone0_vmlinux_size);
  Print(L"====================================================================="
        L"===========\n");

  EFI_STATUS status;
  status = GetMemoryMapAndExitBootServices(ImageHandle, SystemTable);

  // now we have exited boot services
  memcpy2((void *)hvisor_bin_addr, (void *)hvisor_bin_start, hvisor_bin_size);
  memcpy2((void *)hvisor_dtb_addr, (void *)hvisor_dtb_start, hvisor_dtb_size);
  memcpy2((void *)hvisor_zone0_vmlinux_addr, (void *)hvisor_zone0_vmlinux_start,
          hvisor_zone0_vmlinux_size);

  init_serial();

  print_str("Ok, ready to jump to hvisor entry...\n");

  //  place dtb addr in r5
  // as constant
  asm volatile("li.d $r5, %0" ::"i"(hvisor_dtb_addr));

  void *hvisor_entry = (void *)hvisor_bin_addr;
  ((void (*)(void))hvisor_entry)();

  while (1) {
  }

  return EFI_SUCCESS;
}