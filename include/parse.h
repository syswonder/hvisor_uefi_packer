/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include "core.h"

UINTN parse_pe(UINTN efi_file_start_addr, UINTN efi_load_addr, UINTN efi_size);