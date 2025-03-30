/*
 * Copyright 2025 wheatfox
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "core.h"

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

// PL011 UART registers
#define UART_BASE 0x9000000
#define UART_FR 0x18          // Flag register
#define UART_FR_TXFF (1 << 5) // Transmit FIFO full
#define UART_DR 0x00          // Data register

static inline void mmio_write(uint64_t reg, uint32_t val) {
  *(volatile uint32_t *)(reg) = val;
}

static inline uint32_t mmio_read(uint64_t reg) {
  return *(volatile uint32_t *)(reg);
}

void print_char(char c) {
  while (mmio_read(UART_BASE + UART_FR) & UART_FR_TXFF)
    ;
  mmio_write(UART_BASE + UART_DR, c);
}

void arch_init() {}
void init_serial() {}
void set_dmw() {}

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

void print_chars(char *c, int n) {
  for (int j = 0; j < n; j++) {
    char c1 = c[j];
    // if not printable or is \n \r, etc. special char, print '?'
    c1 = (c1 >= 32 && c1 <= 126) || c1 == '\n' || c1 == '\r' ? c1 : '?';
    Print(L"%c", c1);
  }
}