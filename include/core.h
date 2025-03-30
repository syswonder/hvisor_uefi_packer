/*
 * Copyright 2025 wheatfox
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <efi.h>
#include <efilib.h>

void hvisor_bin_start();
void hvisor_bin_end();

void hvisor_zone0_vmlinux_start();
void hvisor_zone0_vmlinux_end();

void set_dmw();
void arch_init();
void init_serial();
void put_char(char c);
void print_str(const char *str);
void print_hex(UINT8 n);
void print_chars(char *c, int n);

char *get_efi_status_string(EFI_STATUS status);
char *get_arch();

void memcpy2(void *dest, void *src, int n);
void memset2(void *dest, int val, int n);

void halt();
void check();