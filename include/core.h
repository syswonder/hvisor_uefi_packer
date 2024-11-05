#pragma once

void hvisor_bin_start();
void hvisor_bin_end();

// void hvisor_dtb_start();
// void hvisor_dtb_end();

void hvisor_zone0_vmlinux_start();
void hvisor_zone0_vmlinux_end();

void set_dmw();
void arch_init();
void init_serial();
void put_char(char c);