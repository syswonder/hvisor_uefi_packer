// Copyright 2025 Syswonder
// SPDX-License-Identifier: MulanPSL-2.0

#include "acpi.h"
#include "core.h"

EFI_GUID gEfiAcpiTableProtocolGuid = EFI_ACPI_TABLE_PROTOCOL_GUID;

// parse acpi aml binary code
// it should begin with "DSDT" because the
// whole DSDT table should be an "AML code"
void parse_aml(char *aml, int size) {
  return;
  print_str("[INFO] parse_aml: dumping AML code to UART:\n");
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
    Print(L"[ERROR] acpi_dump: LocateProtocol failed\n");
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
      Print(L"[INFO] acpi_dump: ACPI table found, version=%d\n",
            CompareGuid(&ect[i].VendorGuid, &Acpi2TableGuid) ? 2 : 1);
      AcpiTable = ect[i].VendorTable;
      Print(L"[INFO] acpi_dump: ACPI table address: 0x%lx, "
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
        Print(L"[INFO] acpi_dump: RSDP found\n");
        Print(L"[INFO] acpi_dump: RSDP address: 0x%lx\n", (UINT64)rsdp);
        if (rsdp->Revision >= 2) {
          Print(L"[INFO] acpi_dump: XSDT address: 0x%lx\n", rsdp->XsdtAddress);
          xsdt = (ACPI_XSDT *)rsdp->XsdtAddress;
        } else {
          Print(L"[INFO] acpi_dump: RSDT address: 0x%lx\n", rsdp->RsdtAddress);
          rsdt = (ACPI_RSDT *)rsdp->RsdtAddress;
        }
      }
    } else if (CompareGuid(&ect[i].VendorGuid, &EfiDtbTableGuid)) {
      Print(L"[INFO] acpi_dump: EFI DTB table found\n");
      Print(L"[INFO] acpi_dump: EFI DTB table address: 0x%lx\n",
            (UINT64)ect[i].VendorTable);
    }
  }

  // Find FADT (Fixed ACPI Description Table)
  // then get DSDT's AML code(ACPI Machine Language)

  // then dump XSDT
  // FADT's signature is "FACP"
  ACPI_FADT *fadt = NULL;

  if (xsdt != NULL) {
    Print(L"[INFO] acpi_dump: XSDT found\n");
    Print(L"[INFO] acpi_dump: XSDT address: 0x%lx\n", (UINT64)xsdt);
    Print(L"[INFO] acpi_dump: XSDT signature: ");
    for (int j = 0; j < 4; j++) {
      Print(L"%c", xsdt->Header.Signature[j]);
    }
    Print(L"\n");
    Print(L"[INFO] acpi_dump: XSDT length: %d\n", xsdt->Header.Length);
    Print(L"[INFO] acpi_dump: XSDT revision: %d\n", xsdt->Header.Revision);
    Print(L"[INFO] acpi_dump: XSDT checksum: %d\n", xsdt->Header.Checksum);
    Print(L"[INFO] acpi_dump: XSDT OEM ID: ");
    for (int j = 0; j < 6; j++) {
      Print(L"%c", xsdt->Header.OemId[j]);
    }
    Print(L"\n");
    Print(L"[INFO] acpi_dump: XSDT OEM Table ID: ");
    for (int j = 0; j < 8; j++) {
      Print(L"%c", xsdt->Header.OemTableId[j]);
    }
    Print(L"\n");
    Print(L"[INFO] acpi_dump: XSDT OEM Revision: %d\n",
          xsdt->Header.OemRevision);
    Print(L"[INFO] acpi_dump: XSDT Creator ID: %d\n", xsdt->Header.CreatorId);
    Print(L"[INFO] acpi_dump: XSDT Creator Revision: %d\n",
          xsdt->Header.CreatorRevision);
    // dump XSDT entries
    for (int i = 0; i < (xsdt->Header.Length - sizeof(ACPI_TABLE_HEADER)) / 8;
         i++) {
      UINT64 *entry = (UINT64 *)xsdt->TableEntries[i];
      Print(L"[INFO] acpi_dump: XSDT entry %d: 0x%lx\n", i, (UINT64)entry);
      // dump first 16 bytes of ACPI table as
      Print(L"[INFO] acpi_dump: XSDT entry %d signature: ", i);
      print_chars((char *)entry, 4);
      Print(L"\n");
      if (CompareMem(entry, "FACP", 4) == 0) {
        fadt = (ACPI_FADT *)entry;
        Print(L"[INFO] acpi_dump: FADT found\n");
        Print(L"[INFO] acpi_dump: FADT address: 0x%lx\n", (UINT64)fadt);
      }
    }
  }

  ACPI_DSDT *dsdt = NULL;
  // dump FADT's DSDT addr
  if (fadt != NULL) {
    Print(L"[INFO] acpi_dump: DSDT address: 0x%lx\n", (UINT64)fadt->Dsdt);
    dsdt = (ACPI_DSDT *)fadt->Dsdt;
  }

  // dump DSDT basic info and AML code size
  if (dsdt != NULL) {
    Print(L"[INFO] acpi_dump: DSDT signature: ");
    for (int j = 0; j < 4; j++) {
      Print(L"%c", dsdt->Header.Signature[j]);
    }
    Print(L"\n");
    Print(L"[INFO] acpi_dump: DSDT length: %d\n", dsdt->Header.Length);
    Print(L"[INFO] acpi_dump: DSDT revision: %d\n", dsdt->Header.Revision);
    Print(L"[INFO] acpi_dump: DSDT checksum: %d\n", dsdt->Header.Checksum);
    // code size
    UINT32 code_size = dsdt->Header.Length - sizeof(ACPI_TABLE_HEADER);
    Print(L"[INFO] acpi_dump: DSDT code size: %d\n", code_size);
  }
  parse_aml((char *)dsdt,
            dsdt->Header.Length); // dump DSDT's AML code(ACPI Machine Language)

  return EFI_SUCCESS;
}