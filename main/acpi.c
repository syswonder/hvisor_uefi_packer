// Copyright 2025 Syswonder
// SPDX-License-Identifier: MulanPSL-2.0

#include "acpi.h"
#include "core.h"

EFI_GUID gEfiAcpiTableProtocolGuid = EFI_ACPI_TABLE_PROTOCOL_GUID;

// Helper function to dump AML code to UART
void parse_aml(char *aml, int size) {
  if (!aml || size <= 0) {
    return;
  }

  print_str("[INFO] parse_aml: dumping AML code to UART:\n");
  // Writing a parser for AML is very complex
  // We just dump each byte as hex
  const int mod = 32;
  for (int i = 0; i < size; i++) {
    print_hex((UINT8)aml[i]);
    if ((i + 1) % mod == 0) {
      print_str("\n\r");
    }
  }
  // If not aligned to mod, print a newline
  if (size % mod != 0) {
    print_str("\n\r");
  }
}

// Helper function to find ACPI table in configuration table
static EFI_STATUS find_acpi_table(EFI_SYSTEM_TABLE *SystemTable,
                                  void **AcpiTable, int *version) {
  EFI_GUID Acpi2TableGuid = ACPI_20_TABLE_GUID; // ACPI 2.0
  EFI_GUID Acpi1TableGuid = ACPI_TABLE_GUID;    // ACPI 1.0
  EFI_CONFIGURATION_TABLE *ect = SystemTable->ConfigurationTable;

  for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
    if (CompareGuid(&ect[i].VendorGuid, &Acpi2TableGuid)) {
      Print(L"[INFO] acpi_dump: ACPI table found, version=2\n");
      *AcpiTable = ect[i].VendorTable;
      *version = 2;
      return EFI_SUCCESS;
    } else if (CompareGuid(&ect[i].VendorGuid, &Acpi1TableGuid)) {
      Print(L"[INFO] acpi_dump: ACPI table found, version=1\n");
      *AcpiTable = ect[i].VendorTable;
      *version = 1;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

// Helper function to dump table signature
static void dump_table_signature(void *table, const char *table_name) {
  Print(L"[INFO] acpi_dump: %a address: 0x%lx, signature: ", table_name,
        (UINT64)table);
  // Dump first 8 bytes of ACPI table as char
  for (int j = 0; j < 8; j++) {
    char c = ((char *)table)[j];
    // If not printable or is \n \r, etc. special char, print '?'
    char c1 = (c >= 32 && c <= 126) || c == '\n' || c == '\r' ? c : '?';
    Print(L"%c", c1);
  }
  Print(L"\n");
}

// Helper function to find RSDP and related tables
static EFI_STATUS find_rsdp_tables(void *AcpiTable, ACPI_RSDP **rsdp,
                                   ACPI_RSDT **rsdt, ACPI_XSDT **xsdt) {
  // RSDP "RSD PTR " signature
  if (CompareMem(AcpiTable, "RSD PTR ", 8) == 0) {
    *rsdp = AcpiTable;
    Print(L"[INFO] acpi_dump: RSDP found\n");
    Print(L"[INFO] acpi_dump: RSDP address: 0x%lx\n", (UINT64)*rsdp);

    if ((*rsdp)->Revision >= 2) {
      Print(L"[INFO] acpi_dump: XSDT address: 0x%lx\n", (*rsdp)->XsdtAddress);
      *xsdt = (ACPI_XSDT *)(*rsdp)->XsdtAddress;
    } else {
      Print(L"[INFO] acpi_dump: RSDT address: 0x%lx\n", (*rsdp)->RsdtAddress);
      *rsdt = (ACPI_RSDT *)(UINTN)(*rsdp)->RsdtAddress;
    }
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

// Helper function to dump XSDT information
static void dump_xsdt_info(ACPI_XSDT *xsdt) {
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
  Print(L"[INFO] acpi_dump: XSDT OEM Revision: %d\n", xsdt->Header.OemRevision);
  Print(L"[INFO] acpi_dump: XSDT Creator ID: %d\n", xsdt->Header.CreatorId);
  Print(L"[INFO] acpi_dump: XSDT Creator Revision: %d\n",
        xsdt->Header.CreatorRevision);
}

// Helper function to find FADT in XSDT entries
static ACPI_FADT *find_fadt_in_xsdt(ACPI_XSDT *xsdt) {
  ACPI_FADT *fadt = NULL;

  // Dump XSDT entries
  for (UINTN i = 0; i < (xsdt->Header.Length - sizeof(ACPI_TABLE_HEADER)) / 8;
       i++) {
    UINT64 *entry = (UINT64 *)xsdt->TableEntries[i];
    Print(L"[INFO] acpi_dump: XSDT entry %d: 0x%lx\n", i, (UINT64)entry);
    // Dump first 4 bytes of ACPI table as signature
    Print(L"[INFO] acpi_dump: XSDT entry %d signature: ", i);
    print_chars((char *)entry, 4);
    Print(L"\n");

    if (CompareMem(entry, "FACP", 4) == 0) {
      fadt = (ACPI_FADT *)entry;
      Print(L"[INFO] acpi_dump: FADT found\n");
      Print(L"[INFO] acpi_dump: FADT address: 0x%lx\n", (UINT64)fadt);
      break;
    }
  }

  return fadt;
}

// Helper function to dump DSDT information
static void dump_dsdt_info(ACPI_DSDT *dsdt) {
  if (!dsdt) {
    return;
  }

  Print(L"[INFO] acpi_dump: DSDT signature: ");
  for (int j = 0; j < 4; j++) {
    Print(L"%c", dsdt->Header.Signature[j]);
  }
  Print(L"\n");
  Print(L"[INFO] acpi_dump: DSDT length: %d\n", dsdt->Header.Length);
  Print(L"[INFO] acpi_dump: DSDT revision: %d\n", dsdt->Header.Revision);
  Print(L"[INFO] acpi_dump: DSDT checksum: %d\n", dsdt->Header.Checksum);

  // Code size
  UINT32 code_size = dsdt->Header.Length - sizeof(ACPI_TABLE_HEADER);
  Print(L"[INFO] acpi_dump: DSDT code size: %d\n", code_size);
}

EFI_STATUS acpi_dump(EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS status;
  EFI_DEVICE_PATH_PROTOCOL *dp_protocol;
  void *AcpiTable = NULL;
  int version = 0;
  ACPI_RSDP *rsdp = NULL;
  ACPI_RSDT *rsdt = NULL;
  ACPI_XSDT *xsdt = NULL;
  ACPI_FADT *fadt = NULL;
  ACPI_DSDT *dsdt = NULL;

  // Locate the Device Path Protocol
  status = uefi_call_wrapper(SystemTable->BootServices->LocateProtocol, 3,
                             &DevicePathProtocol, NULL, (void **)&dp_protocol);
  if (EFI_ERROR(status)) {
    Print(L"[ERROR] acpi_dump: LocateProtocol failed\n");
    halt();
  }

  // Find ACPI table
  status = find_acpi_table(SystemTable, &AcpiTable, &version);
  if (EFI_ERROR(status)) {
    Print(L"[ERROR] acpi_dump: ACPI table not found\n");
    return status;
  }

  dump_table_signature(AcpiTable, "ACPI table");

  // Find RSDP and related tables
  status = find_rsdp_tables(AcpiTable, &rsdp, &rsdt, &xsdt);
  if (EFI_ERROR(status)) {
    Print(L"[ERROR] acpi_dump: RSDP not found\n");
    return status;
  }

  // Check for DTB table
  EFI_GUID EfiDtbTableGuid = EFI_DTB_TABLE_GUID;
  EFI_CONFIGURATION_TABLE *ect = SystemTable->ConfigurationTable;
  for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
    if (CompareGuid(&ect[i].VendorGuid, &EfiDtbTableGuid)) {
      Print(L"[INFO] acpi_dump: EFI DTB table found\n");
      Print(L"[INFO] acpi_dump: EFI DTB table address: 0x%lx\n",
            (UINT64)ect[i].VendorTable);
      break;
    }
  }

  // Find FADT (Fixed ACPI Description Table)
  if (xsdt != NULL) {
    dump_xsdt_info(xsdt);
    fadt = find_fadt_in_xsdt(xsdt);
  }

  // Get DSDT from FADT
  if (fadt != NULL) {
    Print(L"[INFO] acpi_dump: DSDT address: 0x%lx\n", (UINT64)fadt->Dsdt);
    dsdt = (ACPI_DSDT *)(UINTN)fadt->Dsdt;
  }

  // Dump DSDT information and AML code
  dump_dsdt_info(dsdt);
  if (dsdt != NULL) {
    parse_aml((char *)dsdt, dsdt->Header.Length);
  }

  return EFI_SUCCESS;
}