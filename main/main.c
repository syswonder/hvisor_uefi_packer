/*
  Copyright (c) 2024 wheatfox
  hvisor_uefi_packer is licensed under Mulan PSL v2.
  You can use this software according to the terms and conditions of the Mulan
  PSL v2. You may obtain a copy of Mulan PSL v2 at:
  http://license.coscl.org.cn/MulanPSL2
  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
  EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
  MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
  See the Mulan PSL v2 for more details.
*/

#include "core.h"
#include <efi.h>
#include <efilib.h>

#include "acpi.h"

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

#if defined(CONFIG_TARGET_ARCH_AARCH64)

void print_char(char _) {}
void init_serial() {}
void set_dmw() {}
void arch_init() {}

#elif defined(CONFIG_TARGET_ARCH_LOONGARCH64)

void print_char(char c);

#else
#error "Unsupported target arch"
#endif

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

char *get_arch() {
#if defined(CONFIG_TARGET_ARCH_AARCH64)
  return "aarch64";
#elif defined(CONFIG_TARGET_ARCH_LOONGARCH64)
  return "loongarch64";
#else
#error "Unsupported target arch"
#endif
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

EFI_STATUS exit_boot_servies(EFI_HANDLE ImageHandle,
                             EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS status;
  // get memory map
  memory_map_size = 0;
  memory_map_desc = NULL;
  status = uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5,
                             &memory_map_size, memory_map_desc, &map_key,
                             &desc_size, &desc_version);

  check(status, "GetMemoryMap (1st call)", EFI_BUFFER_TOO_SMALL, SystemTable);
  Print(L"[INFO] (exit_boot_servies) memory_map_size = %ld\n", memory_map_size);

  memory_map_size += 20 * desc_size;
  status = uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3,
                             EfiLoaderData, memory_map_size,
                             (void **)&memory_map_desc);
  if (memory_map_desc == NULL) {
    Print(L"[ERROR] (exit_boot_servies) AllocatePool failed\n");
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

void print_chars(char *c, int n) {
  for (int j = 0; j < n; j++) {
    char c1 = c[j];
    // if not printable or is \n \r, etc. special char, print '?'
    c1 = (c1 >= 32 && c1 <= 126) || c1 == '\n' || c1 == '\r' ? c1 : '?';
    Print(L"%c", c1);
  }
}

// parse acpi aml binary code
// it should begin with "DSDT" because the
// whole DSDT table should be an "AML code"
void parse_aml(char *aml, int size) {
  print_str("[INFO] (parse_aml) dumping AML code to UART:\n");
  // writing a parser for AML is very complex
  // we just dump each byte as hex
  const int mod = 32;
  for (int i = 0; i < size; i++) {
    // Print(L"%02x", (UINT8)aml[i]);
    print_hex((UINT8)aml[i]);
    if ((i + 1) % mod == 0) {
      // Print(L"\n");
      print_str("\n\r");
    }
  }
  // if not aligned to mod, print a newline
  if (size % mod != 0) {
    // Print(L"\n");
    print_str("\n\r");
  }
}

EFI_STATUS acpi_dump(EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS status;
  EFI_DEVICE_PATH_PROTOCOL *dp_protocol;

  // Locate the Device Path Protocol
  status = uefi_call_wrapper(SystemTable->BootServices->LocateProtocol, 3,
                             &DevicePathProtocol, NULL, (void **)&dp_protocol);
  if (EFI_ERROR(status)) {
    Print(L"[ERROR] (acpi_dump) LocateProtocol failed\n");
    halt();
  }

  // gnu-efi lacks ACPI protocol, so retrieve the table manually
  EFI_GUID Acpi2TableGuid = ACPI_20_TABLE_GUID; // ACPI 2.0
  EFI_GUID Acpi1TableGuid = ACPI_TABLE_GUID;    // ACPI 1.0
  EFI_GUID EfiDtbTableGuid = EFI_DTB_TABLE_GUID;

  ACPI_RSDP *rsdp = NULL;
  ACPI_RSDT *rsdt = NULL;
  ACPI_XSDT *xsdt = NULL;

  EFI_CONFIGURATION_TABLE *ect = SystemTable->ConfigurationTable;
  void *AcpiTable = NULL;

  for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
    if (CompareGuid(&ect[i].VendorGuid, &Acpi2TableGuid) ||
        CompareGuid(&ect[i].VendorGuid, &Acpi1TableGuid)) {
      Print(L"[INFO] (acpi_dump) ACPI table found, version=%d\n",
            CompareGuid(&ect[i].VendorGuid, &Acpi2TableGuid) ? 2 : 1);
      AcpiTable = ect[i].VendorTable;
      Print(L"[INFO] (acpi_dump) ACPI table address: 0x%lx, "
            L"signature: ",
            (UINT64)AcpiTable);
      // dump first 16 bytes of ACPI table as char
      for (int j = 0; j < 8; j++) {
        char c = ((char *)AcpiTable)[j];
        // if not printable or is \n \r, etc. special char, print '?'
        char c1 = (c >= 32 && c <= 126) || c == '\n' || c == '\r' ? c : '?';
        Print(L"%c", c1);
      }
      Print(L"\n");
      // RSDP "RSD PTR " signature
      if (CompareMem(AcpiTable, "RSD PTR ", 8) == 0) {
        rsdp = AcpiTable;
        Print(L"[INFO] (acpi_dump) RSDP found\n");
        Print(L"[INFO] (acpi_dump) RSDP address: 0x%lx\n", (UINT64)rsdp);
        if (rsdp->Revision >= 2) {
          Print(L"[INFO] (acpi_dump) XSDT address: 0x%lx\n", rsdp->XsdtAddress);
          xsdt = (ACPI_XSDT *)rsdp->XsdtAddress;
        } else {
          Print(L"[INFO] (acpi_dump) RSDT address: 0x%lx\n", rsdp->RsdtAddress);
          rsdt = (ACPI_RSDT *)rsdp->RsdtAddress;
        }
      }
    } else if (CompareGuid(&ect[i].VendorGuid, &EfiDtbTableGuid)) {
      Print(L"[INFO] (acpi_dump) EFI DTB table found\n");
      Print(L"[INFO] (acpi_dump) EFI DTB table address: 0x%lx\n",
            (UINT64)ect[i].VendorTable);
    }
  }

  // Find FADT (Fixed ACPI Description Table)
  // then get DSDT's AML code(ACPI Machine Language)

  // then dump XSDT
  // FADT's signature is "FACP"
  ACPI_FADT *fadt = NULL;

  if (xsdt != NULL) {
    Print(L"[INFO] (acpi_dump) XSDT found\n");
    Print(L"[INFO] (acpi_dump) XSDT address: 0x%lx\n", (UINT64)xsdt);
    Print(L"[INFO] (acpi_dump) XSDT signature: ");
    for (int j = 0; j < 4; j++) {
      Print(L"%c", xsdt->Header.Signature[j]);
    }
    Print(L"\n");
    Print(L"[INFO] (acpi_dump) XSDT length: %d\n", xsdt->Header.Length);
    Print(L"[INFO] (acpi_dump) XSDT revision: %d\n", xsdt->Header.Revision);
    Print(L"[INFO] (acpi_dump) XSDT checksum: %d\n", xsdt->Header.Checksum);
    Print(L"[INFO] (acpi_dump) XSDT OEM ID: ");
    for (int j = 0; j < 6; j++) {
      Print(L"%c", xsdt->Header.OemId[j]);
    }
    Print(L"\n");
    Print(L"[INFO] (acpi_dump) XSDT OEM Table ID: ");
    for (int j = 0; j < 8; j++) {
      Print(L"%c", xsdt->Header.OemTableId[j]);
    }
    Print(L"\n");
    Print(L"[INFO] (acpi_dump) XSDT OEM Revision: %d\n",
          xsdt->Header.OemRevision);
    Print(L"[INFO] (acpi_dump) XSDT Creator ID: %d\n", xsdt->Header.CreatorId);
    Print(L"[INFO] (acpi_dump) XSDT Creator Revision: %d\n",
          xsdt->Header.CreatorRevision);
    // dump XSDT entries
    for (int i = 0; i < (xsdt->Header.Length - sizeof(ACPI_TABLE_HEADER)) / 8;
         i++) {
      UINT64 *entry = (UINT64 *)xsdt->TableEntries[i];
      Print(L"[INFO] (acpi_dump) XSDT entry %d: 0x%lx\n", i, (UINT64)entry);
      // dump first 16 bytes of ACPI table as
      Print(L"[INFO] (acpi_dump) XSDT entry %d signature: ", i);
      print_chars((char *)entry, 4);
      Print(L"\n");
      if (CompareMem(entry, "FACP", 4) == 0) {
        fadt = (ACPI_FADT *)entry;
        Print(L"[INFO] (acpi_dump) FADT found\n");
        Print(L"[INFO] (acpi_dump) FADT address: 0x%lx\n", (UINT64)fadt);
      }
    }
  }

  ACPI_DSDT *dsdt = NULL;
  // dump FADT's DSDT addr
  if (fadt != NULL) {
    Print(L"[INFO] (acpi_dump) DSDT address: 0x%lx\n", (UINT64)fadt->Dsdt);
    dsdt = (ACPI_DSDT *)fadt->Dsdt;
  }

  // dump DSDT basic info and AML code size
  if (dsdt != NULL) {
    Print(L"[INFO] (acpi_dump) DSDT signature: ");
    for (int j = 0; j < 4; j++) {
      Print(L"%c", dsdt->Header.Signature[j]);
    }
    Print(L"\n");
    Print(L"[INFO] (acpi_dump) DSDT length: %d\n", dsdt->Header.Length);
    Print(L"[INFO] (acpi_dump) DSDT revision: %d\n", dsdt->Header.Revision);
    Print(L"[INFO] (acpi_dump) DSDT checksum: %d\n", dsdt->Header.Checksum);
    // code size
    UINT32 code_size = dsdt->Header.Length - sizeof(ACPI_TABLE_HEADER);
    Print(L"[INFO] (acpi_dump) DSDT code size: %d\n", code_size);
  }
  parse_aml((char *)dsdt,
            dsdt->Header.Length); // dump DSDT's AML code(ACPI Machine Language)

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {

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

  acpi_dump(SystemTable);

  // the entry is the same as the load address
  UINTN hvisor_bin_size = &hvisor_bin_end - &hvisor_bin_start;
  // UINTN hvisor_dtb_size = &hvisor_dtb_end - &hvisor_dtb_start;
  UINTN hvisor_zone0_vmlinux_size =
      &hvisor_zone0_vmlinux_end - &hvisor_zone0_vmlinux_start;

  // TODO: add to Kconfig
#if defined(CONFIG_TARGET_ARCH_LOONGARCH64)
  const UINTN hvisor_bin_addr = 0x9000000100010000ULL;
  const UINTN hvisor_zone0_vmlinux_addr =
      0x9000000000200000ULL; // caution: this is actually a vmlinux.bin, not a
                             // vmlinux - wheatfox
  const UINTN memset_st = 0x9000000100000000ULL;
  const UINTN memset_ed = 0x9000000101000000ULL;
  const UINTN memset2_st = 0x9000000000000000ULL + 0x1000;
  const UINTN memset2_size = 0x10000;
#elif defined(CONFIG_TARGET_ARCH_AARCH64)
  const UINTN hvisor_bin_addr = 0x40000000;
  const UINTN hvisor_zone0_vmlinux_addr = 0x40200000;
#else
#error "Unsupported target arch"
#endif

#if defined(CONFIG_TARGET_ARCH_LOONGARCH64)
  Print(L"[INFO] clearing memory from 0x%lx to 0x%lx\n", memset_st, memset_ed);
  memset2((void *)memset_st, 0, memset_ed - memset_st);
  // check memset
  for (UINTN i = memset_st; i < memset_ed; i++) {
    if (*(UINT8 *)i != 0) {
      Print(L"memset failed at 0x%lx\n", i);
      halt();
    }
  }
  Print(L"[INFO] clearing memory from 0x%lx to 0x%lx\n", memset2_st,
        memset2_st + memset2_size);
  memset2((void *)memset2_st, 0, memset2_size);
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

  EFI_STATUS status;
  status = exit_boot_servies(ImageHandle, SystemTable);

  // now we have exited boot services
  memcpy2((void *)hvisor_bin_addr, (void *)hvisor_bin_start, hvisor_bin_size);
  // memcpy2((void *)hvisor_dtb_addr, (void *)hvisor_dtb_start,
  // hvisor_dtb_size);
  memcpy2((void *)hvisor_zone0_vmlinux_addr, (void *)hvisor_zone0_vmlinux_start,
          hvisor_zone0_vmlinux_size);

  init_serial();
  print_str("[INFO] Ok, ready to jump to hvisor entry...\n");

  //  place dtb addr in r5
  // as constant
  // asm volatile("li.d $r5, %0" ::"i"(hvisor_dtb_addr));

  void *hvisor_entry = (void *)hvisor_bin_addr;
  ((void (*)(void))hvisor_entry)();

  while (1) {
  }

  return EFI_SUCCESS;
}