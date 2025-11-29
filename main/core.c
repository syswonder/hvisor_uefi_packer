/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "core.h"
#include "arch.h"

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
  Print(L"[INFO] halt: halting system\n");
  while (1)
    ;
}

void print_char(char c) {
  if (arch_ops != NULL) {
    ARCH_PUT_CHAR(c);
  }
}

void print_str(const char *str) {
  while (*str) {
    print_char(*str);
    str++;
  }
}

void print_hex(UINT8 n) {
  UINT8 c = n >> 4;
  print_char(c > 9 ? c - 10 + 'A' : c + '0');
  c = n & 0xf;
  print_char(c > 9 ? c - 10 + 'A' : c + '0');
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

const char *get_arch() {
  if (arch_ops != NULL) {
    return ARCH_NAME();
  }
  return "unknown";
}

void check(EFI_STATUS status, const char *prefix, EFI_STATUS expected,
           EFI_SYSTEM_TABLE *SystemTable) {
  if (status != expected) {
    uefi_call_wrapper(SystemTable->ConOut->SetAttribute, 2, SystemTable->ConOut,
                      EFI_LIGHTRED);
    Print(L"[ERROR] check: %a failed, should be %a, but got %a\n", prefix,
          get_efi_status_string(expected), get_efi_status_string(status));
    uefi_call_wrapper(SystemTable->ConOut->SetAttribute, 2, SystemTable->ConOut,
                      EFI_LIGHTGRAY);
    halt();
  }
  Print(L"[INFO] check: %a success\n", prefix);
}

void print_chars(char *c, int n) {
  for (int j = 0; j < n; j++) {
    char c1 = c[j];
    // if not printable or is \n \r, etc. special char, print '?'
    c1 = (c1 >= 32 && c1 <= 126) || c1 == '\n' || c1 == '\r' ? c1 : '?';
    Print(L"%c", c1);
  }
}

// UEFI boot services management
static UINTN memory_map_size = 0;
static UINTN map_key, desc_size;
static UINT32 desc_version;
static EFI_MEMORY_DESCRIPTOR *memory_map_desc;

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