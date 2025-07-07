/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <efi.h>
#include <efilib.h>

extern void hvisor_bin_start();
extern void hvisor_bin_end();

#if defined(CONFIG_ENABLE_VMLINUX)
extern void hvisor_zone0_vmlinux_start();
extern void hvisor_zone0_vmlinux_end();
#endif

void print_str(const char *str);
void print_hex(UINT8 n);
void print_chars(char *c, int n);

char *get_efi_status_string(EFI_STATUS status);
const char *get_arch();

void memcpy2(void *dest, void *src, int n);
void memset2(void *dest, int val, int n);

void halt();
void check(EFI_STATUS status, const char *prefix, EFI_STATUS expected,
           EFI_SYSTEM_TABLE *SystemTable);

EFI_STATUS exit_boot_services(EFI_HANDLE ImageHandle,
                              EFI_SYSTEM_TABLE *SystemTable);