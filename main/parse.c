/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "core.h"
#include "pe.h"

// Helper function to convert size to human readable string
CHAR16 *convert_size_to_str(UINTN size) {
  static CHAR16 size_str[32];
  if (size < 1024) {
    SPrint(size_str, sizeof(size_str), L"%llu B", size);
  } else if (size < 1024 * 1024) {
    SPrint(size_str, sizeof(size_str), L"%llu KB", size / 1024);
  } else if (size < 1024 * 1024 * 1024) {
    SPrint(size_str, sizeof(size_str), L"%llu MB", size / (1024 * 1024));
  } else {
    SPrint(size_str, sizeof(size_str), L"%llu GB", size / (1024 * 1024 * 1024));
  }
  return size_str;
}

// Helper function to parse section name
CHAR16 *parse_section_name(UINT8 *name, UINTN size) {
  static CHAR16 section_name[IMAGE_SIZEOF_SHORT_NAME + 1];
  UINTN i;

  for (i = 0; i < size && i < IMAGE_SIZEOF_SHORT_NAME; i++) {
    if (name[i] == '\0') {
      break;
    }
    section_name[i] = name[i];
  }
  section_name[i] = '\0';
  return section_name;
}

// Helper function to validate DOS header
static EFI_STATUS validate_dos_header(IMAGE_DOS_HEADER *dos_header) {
  if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
    Print(L"[ERROR] parse_pe: invalid dos header: e_magic = 0x%x\n",
          dos_header->e_magic);
    return EFI_INVALID_PARAMETER;
  }
  Print(L"[INFO] parse_pe: dos header is ok: e_magic = 0x%x\n",
        dos_header->e_magic);
  return EFI_SUCCESS;
}

// Helper function to validate NT header
static EFI_STATUS validate_nt_header(IMAGE_NT_HEADERS *nt_header) {
  if (nt_header->Signature != IMAGE_NT_SIGNATURE) {
    Print(L"[ERROR] parse_pe: invalid nt header: Signature = 0x%x\n",
          nt_header->Signature);
    return EFI_INVALID_PARAMETER;
  }
  Print(L"[INFO] parse_pe: nt header is ok: Signature = 0x%x\n",
        nt_header->Signature);
  return EFI_SUCCESS;
}

// Helper function to print optional header info
static void print_optional_header_info(IMAGE_OPTIONAL_HEADER *optional_header) {
  Print(L"[INFO] parse_pe: optional header:\n");
  Print(L"[INFO] parse_pe:   SizeOfCode = %d\n", optional_header->SizeOfCode);
  Print(L"[INFO] parse_pe:   SizeOfInitializedData = %d\n",
        optional_header->SizeOfInitializedData);
  Print(L"[INFO] parse_pe:   SizeOfUninitializedData = %d\n",
        optional_header->SizeOfUninitializedData);
  Print(L"[INFO] parse_pe:   AddressOfEntryPoint = 0x%lx\n",
        optional_header->AddressOfEntryPoint);
  Print(L"[INFO] parse_pe:   BaseOfCode = 0x%lx\n",
        optional_header->BaseOfCode);
  Print(L"[INFO] parse_pe:   ImageBase = 0x%lx\n", optional_header->ImageBase);
}

// Helper function to load section
static EFI_STATUS load_section(IMAGE_SECTION_HEADER *section_header,
                               UINTN efi_file_start_addr, UINTN efi_load_addr,
                               UINTN efi_size, UINTN section_index) {
  UINTN section_start = efi_file_start_addr + section_header->PointerToRawData;
  UINTN section_size = section_header->SizeOfRawData;
  UINTN section_end = section_start + section_size;
  UINTN section_load_addr = efi_load_addr + section_header->VirtualAddress;

  Print(L"[INFO] parse_pe:   Section %d:\n", section_index);
  Print(L"[INFO] parse_pe:     Name = %s\n",
        parse_section_name(section_header->Name, IMAGE_SIZEOF_SHORT_NAME));
  Print(L"[INFO] parse_pe:     VirtualSize = %d\n",
        section_header->Misc.VirtualSize);
  Print(L"[INFO] parse_pe:     VirtualAddress = 0x%lx\n",
        section_header->VirtualAddress);
  Print(L"[INFO] parse_pe:     SizeOfRawData = %d\n",
        section_header->SizeOfRawData);
  Print(L"[INFO] parse_pe:   section_start = 0x%lx\n", section_start);
  Print(L"[INFO] parse_pe:   section_size = %d\n", section_size);
  Print(L"[INFO] parse_pe:   section_end = 0x%lx\n", section_end);
  Print(L"[INFO] parse_pe:   section_load_addr = 0x%lx\n", section_load_addr);

  // Validate section bounds
  if (section_end > efi_file_start_addr + efi_size) {
    Print(L"[ERROR] parse_pe: section end out of range\n");
    return EFI_INVALID_PARAMETER;
  }

  // Copy section to target location
  CopyMem((VOID *)section_load_addr, (VOID *)section_start, section_size);
  Print(L"[INFO] parse_pe:   memcpy2 done: from 0x%lx to 0x%lx, size = %d\n",
        section_start, section_load_addr, section_size);

  return EFI_SUCCESS;
}

// parse vmlinux.efi which is a PE32+ file
// if anything wrong, return 0
// if success, return the entry point address
UINTN parse_pe(UINTN efi_file_start_addr, UINTN efi_load_addr, UINTN efi_size) {
  EFI_STATUS status;
  IMAGE_DOS_HEADER *dos_header;
  IMAGE_NT_HEADERS *nt_header;
  IMAGE_FILE_HEADER *file_header;
  IMAGE_OPTIONAL_HEADER *optional_header;
  IMAGE_SECTION_HEADER *section_header;
  UINTN pe_offset, entry;

  Print(L"[INFO] parse_pe: efi_file_start_addr = 0x%lx\n", efi_file_start_addr);
  Print(L"[INFO] parse_pe: efi_load_addr = 0x%lx\n", efi_load_addr);
  Print(L"[INFO] parse_pe: efi_size = %llu (%s)\n", efi_size,
        convert_size_to_str(efi_size));

  // Parse DOS header
  dos_header = (IMAGE_DOS_HEADER *)efi_file_start_addr;
  status = validate_dos_header(dos_header);
  if (EFI_ERROR(status)) {
    return 0;
  }

  // Parse NT header
  pe_offset = dos_header->e_lfanew;
  nt_header = (IMAGE_NT_HEADERS *)(efi_file_start_addr + pe_offset);
  status = validate_nt_header(nt_header);
  if (EFI_ERROR(status)) {
    return 0;
  }

  file_header = &nt_header->FileHeader;
  optional_header = &nt_header->OptionalHeader;

  print_optional_header_info(optional_header);

  entry = optional_header->AddressOfEntryPoint + efi_load_addr;

  // Parse section headers
  section_header = (IMAGE_SECTION_HEADER *)((UINTN)&nt_header->OptionalHeader +
                                            file_header->SizeOfOptionalHeader);

  Print(L"[INFO] parse_pe: section header@ 0x%lx:\n", (UINTN)section_header);

  // Load all sections
  for (UINTN i = 0; i < file_header->NumberOfSections; i++) {
    status = load_section(&section_header[i], efi_file_start_addr,
                          efi_load_addr, efi_size, i);
    if (EFI_ERROR(status)) {
      return 0;
    }
  }

  Print(L"[INFO] parse_pe: section header end\n");
  Print(L"[INFO] parse_pe: entry point = 0x%lx\n", entry);

  return entry;
}