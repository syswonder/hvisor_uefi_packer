
/**
 * ACPI related definitions
 * wheatfox 2025.3.1 wheatfox17@icloud.com
 */
#ifndef _ACPI_H_
#define _ACPI_H_

#include <efi.h>

// Define ACPI RSDP structure (Root System Description Pointer)
typedef struct {
  CHAR8 Signature[8]; // "RSD PTR "
  UINT8 Checksum;
  CHAR8 OemId[6];
  UINT8 Revision;     // 0 = ACPI 1.0, 2 = ACPI 2.0+
  UINT32 RsdtAddress; // RSDT address
  UINT32 Length;      // Total length (only valid if Revision >= 2)
  UINT64 XsdtAddress; // XSDT address (only valid if Revision >= 2)
  UINT8 ExtendedChecksum;
  UINT8 Reserved[3];
} __attribute__((packed)) ACPI_RSDP;

// Define ACPI Table Header
typedef struct {
  CHAR8 Signature[4];
  UINT32 Length;
  UINT8 Revision;
  UINT8 Checksum;
  CHAR8 OemId[6];
  CHAR8 OemTableId[8];
  UINT32 OemRevision;
  UINT32 CreatorId;
  UINT32 CreatorRevision;
} __attribute__((packed)) ACPI_TABLE_HEADER;

typedef struct {
  ACPI_TABLE_HEADER Header;
  UINT64 TableEntries[];
} __attribute__((packed)) ACPI_XSDT;

typedef struct {
  ACPI_TABLE_HEADER Header;
  UINT32 TableEntries[];
} __attribute__((packed)) ACPI_RSDT;

#define EFI_ACPI_TABLE_PROTOCOL_GUID                                           \
  {0xffe06bdd, 0x6107, 0x46a6, {0x7b, 0xb2, 0x5a, 0x9c, 0x7e, 0xc5, 0x27, 0x5c}}

typedef struct _EFI_ACPI_TABLE_PROTOCOL EFI_ACPI_TABLE_PROTOCOL;

EFI_GUID gEfiAcpiTableProtocolGuid = EFI_ACPI_TABLE_PROTOCOL_GUID;

typedef EFI_STATUS(EFIAPI *EFI_ACPI_TABLE_INSTALL_ACPI_TABLE)(
    IN EFI_ACPI_TABLE_PROTOCOL *This, IN VOID *AcpiTableBuffer,
    IN UINTN AcpiTableBufferSize, OUT UINTN *TableKey);

typedef EFI_STATUS(EFIAPI *EFI_ACPI_TABLE_UNINSTALL_ACPI_TABLE)(
    IN EFI_ACPI_TABLE_PROTOCOL *This, IN UINTN TableKey);

typedef struct _EFI_ACPI_TABLE_PROTOCOL {
  EFI_ACPI_TABLE_INSTALL_ACPI_TABLE InstallAcpiTable;
  EFI_ACPI_TABLE_UNINSTALL_ACPI_TABLE UninstallAcpiTable;
} EFI_ACPI_TABLE_PROTOCOL;

// ACPI_GENERIC_ADDRESS
typedef struct {
  UINT8 AddressSpaceId;
  UINT8 RegisterBitWidth;
  UINT8 RegisterBitOffset;
  UINT8 AccessSize;
  UINT64 Address;
} __attribute__((packed)) ACPI_GENERIC_ADDRESS;

// FADT
typedef struct {
  ACPI_TABLE_HEADER Header;
  UINT32 FirmwareCtrl;
  UINT32 Dsdt;
  UINT8 Reserved;
  UINT8 PreferredPmProfile;
  UINT16 SciInt;
  UINT32 SmiCmd;
  UINT8 AcpiEnable;
  UINT8 AcpiDisable;
  UINT8 S4BiosReq;
  UINT8 PstateCnt;
  UINT32 Pm1aEvtBlk;
  UINT32 Pm1bEvtBlk;
  UINT32 Pm1aCntBlk;
  UINT32 Pm1bCntBlk;
  UINT32 Pm2CntBlk;
  UINT32 PmTmrBlk;
  UINT32 Gpe0Blk;
  UINT32 Gpe1Blk;
  UINT8 Pm1EvtLen;
  UINT8 Pm1CntLen;
  UINT8 Pm2CntLen;
  UINT8 PmTmrLen;
  UINT8 Gpe0BlkLen;
  UINT8 Gpe1BlkLen;
  UINT8 Gpe1Base;
  UINT8 CstCnt;
  UINT16 PLvl2Lat;
  UINT16 PLvl3Lat;
  UINT16 FlushSize;
  UINT16 FlushStride;
  UINT8 DutyOffset;
  UINT8 DutyWidth;
  UINT8 DayAlrm;
  UINT8 MonAlrm;
  UINT8 Century;
  UINT16 IaPcBootArch;
  UINT8 Reserved2;
  UINT32 Flags;
  ACPI_GENERIC_ADDRESS ResetReg;
  UINT8 ResetValue;
  UINT8 Reserved3[3];
  UINT64 X_FirmwareCtrl;
  UINT64 X_Dsdt;
  ACPI_GENERIC_ADDRESS X_PM1aEvtBlk;
  ACPI_GENERIC_ADDRESS X_PM1bEvtBlk;
  ACPI_GENERIC_ADDRESS X_PM1aCntBlk;
  ACPI_GENERIC_ADDRESS X_PM1bCntBlk;
  ACPI_GENERIC_ADDRESS X_PM2CntBlk;
  ACPI_GENERIC_ADDRESS X_PMTmrBlk;
  ACPI_GENERIC_ADDRESS X_GPE0Blk;
  ACPI_GENERIC_ADDRESS X_GPE1Blk;
} __attribute__((packed)) ACPI_FADT;

// DSDT
typedef struct {
  ACPI_TABLE_HEADER Header;
  UINT8 AmlCode[];
} __attribute__((packed)) ACPI_DSDT;

#endif // _ACPI_H_