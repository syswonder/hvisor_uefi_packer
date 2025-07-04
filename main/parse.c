/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "core.h"
#include "pe.h"

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

CHAR16 *parse_section_name(UINT8 *name, UINTN size) {
  static CHAR16 section_name[IMAGE_SIZEOF_SHORT_NAME + 1];
  for (UINTN i = 0; i < size; i++) {
    if (name[i] == '\0') {
      break;
    }
    section_name[i] = name[i];
  }
  section_name[size] = '\0';
  return section_name;
}

// parse vmlinux.efi which is a PE32+ file
// if anything wrong, return 0
// if success, return the entry point address
UINTN parse_pe(UINTN efi_file_start_addr, UINTN efi_load_addr, UINTN efi_size) {
  Print(L"[INFO] parse_pe: efi_file_start_addr = 0x%lx\n", efi_file_start_addr);
  Print(L"[INFO] parse_pe: efi_load_addr = 0x%lx\n", efi_load_addr);
  Print(L"[INFO] parse_pe: efi_size = %llu (%s)\n", efi_size,
        convert_size_to_str(efi_size));
  IMAGE_DOS_HEADER *dos_header;

  // parse dos header
  dos_header = (IMAGE_DOS_HEADER *)efi_file_start_addr;
  if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
    Print(L"[ERROR] parse_pe: invalid dos header: e_magic = 0x%x\n",
          dos_header->e_magic);
    return 0;
  } else {
    Print(L"[INFO] parse_pe: dos header is ok: e_magic = 0x%x\n",
          dos_header->e_magic);
  }

  UINTN pe_offset = dos_header->e_lfanew;
  IMAGE_NT_HEADERS *nt_header;
  nt_header = (IMAGE_NT_HEADERS *)(efi_file_start_addr + pe_offset);
  if (nt_header->Signature != IMAGE_NT_SIGNATURE) {
    Print(L"[ERROR] parse_pe: invalid nt header: Signature = 0x%x\n",
          nt_header->Signature);
    return 0;
  } else {
    Print(L"[INFO] parse_pe: nt header is ok: Signature = 0x%x\n",
          nt_header->Signature);
  }

  IMAGE_FILE_HEADER *file_header = &nt_header->FileHeader;
  IMAGE_OPTIONAL_HEADER *optional_header = &nt_header->OptionalHeader;

  // dump file header
//   Print(L"[INFO] parse_pe: file header:\n");
//   Print(L"[INFO] parse_pe:   Machine = 0x%x\n", file_header->Machine);
//   Print(L"[INFO] parse_pe:   NumberOfSections = %d\n",
//         file_header->NumberOfSections);
//   Print(L"[INFO] parse_pe:   TimeDateStamp = %d\n", file_header->TimeDateStamp);
//   Print(L"[INFO] parse_pe:   PointerToSymbolTable = %d\n",
//         file_header->PointerToSymbolTable);
//   Print(L"[INFO] parse_pe:   NumberOfSymbols = %d\n",
//         file_header->NumberOfSymbols);
//   Print(L"[INFO] parse_pe:   SizeOfOptionalHeader = %d\n",
//         file_header->SizeOfOptionalHeader);
//   Print(L"[INFO] parse_pe:   Characteristics = 0x%x\n",
//         file_header->Characteristics);

  // dump optional header
  Print(L"[INFO] parse_pe: optional header:\n");
//   Print(L"[INFO] parse_pe:   Magic = 0x%x\n", optional_header->Magic);
//   Print(L"[INFO] parse_pe:   MajorLinkerVersion = %d\n",
//         optional_header->MajorLinkerVersion);
//   Print(L"[INFO] parse_pe:   MinorLinkerVersion = %d\n",
//         optional_header->MinorLinkerVersion);
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
//   Print(L"[INFO] parse_pe:   SectionAlignment = 0x%lx\n",
//         optional_header->SectionAlignment);
//   Print(L"[INFO] parse_pe:   FileAlignment = 0x%lx\n",
//         optional_header->FileAlignment);
//   // SizeOfImage
//   Print(L"[INFO] parse_pe:   SizeOfImage = %d\n", optional_header->SizeOfImage);
//   Print(L"[INFO] parse_pe:   SizeOfHeaders = %d\n",
//         optional_header->SizeOfHeaders);

  UINTN entry = optional_header->AddressOfEntryPoint + efi_load_addr;

  IMAGE_SECTION_HEADER *section_header;
  section_header = (IMAGE_SECTION_HEADER *)((UINTN)&nt_header->OptionalHeader +
                                            file_header->SizeOfOptionalHeader);

  Print(L"[INFO] parse_pe: section header@ 0x%lx:\n", (UINTN)section_header);
  for (UINTN i = 0; i < file_header->NumberOfSections; i++) {
    Print(L"[INFO] parse_pe:   Section %d:\n", i);
    Print(L"[INFO] parse_pe:     Name = %s\n", parse_section_name(
          section_header[i].Name, IMAGE_SIZEOF_SHORT_NAME));
    Print(L"[INFO] parse_pe:     VirtualSize = %d\n",
          section_header[i].Misc.VirtualSize);
    Print(L"[INFO] parse_pe:     VirtualAddress = 0x%lx\n",
          section_header[i].VirtualAddress);
    Print(L"[INFO] parse_pe:     SizeOfRawData = %d\n",
          section_header[i].SizeOfRawData);
//     Print(L"[INFO] parse_pe:     PointerToRawData = %d\n",
//           section_header[i].PointerToRawData);
//     Print(L"[INFO] parse_pe:     PointerToRelocations = %d\n",
//           section_header[i].PointerToRelocations);
//     Print(L"[INFO] parse_pe:     PointerToLinenumbers = %d\n",
//           section_header[i].PointerToLinenumbers);
//     Print(L"[INFO] parse_pe:     NumberOfRelocations = %d\n",
//           section_header[i].NumberOfRelocations);
//     Print(L"[INFO] parse_pe:     NumberOfLinenumbers = %d\n",
//           section_header[i].NumberOfLinenumbers);
//     Print(L"[INFO] parse_pe:     Characteristics = 0x%x\n",
//           section_header[i].Characteristics);
    // load this section to memory @ efi_load_addr +
    // section_header[i].VirtualAddress
    UINTN section_start =
        efi_file_start_addr + section_header[i].PointerToRawData;
    UINTN section_size = section_header[i].SizeOfRawData;
    UINTN section_end = section_start + section_size;
    UINTN section_load_addr = efi_load_addr + section_header[i].VirtualAddress;
    Print(L"[INFO] parse_pe:   section_start = 0x%lx\n", section_start);
    Print(L"[INFO] parse_pe:   section_size = %d\n", section_size);
    Print(L"[INFO] parse_pe:   section_end = 0x%lx\n", section_end);
    Print(L"[INFO] parse_pe:   section_load_addr = 0x%lx\n", section_load_addr);
    if (section_end > efi_file_start_addr + efi_size) {
      Print(L"[ERROR] parse_pe: section end out of range\n");
      return 0;
    }
    // memcpy2
    CopyMem((VOID *)section_load_addr, (VOID *)section_start, section_size);
    Print(L"[INFO] parse_pe:   memcpy2 done: from 0x%lx to 0x%lx, size = %d\n",
          section_start, section_load_addr, section_size);
  }
  Print(L"[INFO] parse_pe: section header end\n");
  Print(L"[INFO] parse_pe: entry point = 0x%lx\n", entry);

  return entry;
}