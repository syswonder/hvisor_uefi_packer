#include <efi.h>
#include <efilib.h>

void hvisor_bin_start();
void hvisor_bin_end();

void hvisor_dtb_start();
void hvisor_dtb_end();

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  InitializeLib(ImageHandle, SystemTable);
  Print(L"\n");
  Print(L"Hello! This is the UEFI bootloader of hvisor(loongarch)...\n");
  Print(L"hvisor binary stored in .data, from 0x%lx to 0x%lx\n",
        hvisor_bin_start, hvisor_bin_end);
  Print(L"hvisor dtb stored in .data, from 0x%lx to 0x%lx\n",
        hvisor_dtb_start, hvisor_dtb_end);

  // set up 0x8 and 0x9 DMW
  set_dmw();
  arch_init();

  // the entry is the same as the load address

  UINTN hvisor_bin_size = &hvisor_bin_end - &hvisor_bin_start;
  UINTN hvisor_dtb_size = &hvisor_dtb_end - &hvisor_dtb_start;

  const UINTN hvisor_bin_addr = 0x9000000080000000ULL;
  const UINTN hvisor_dtb_addr = 0x9000000090000000ULL;

  // // store 2333 to hvisor_bin_addr
  // *(UINTN *)hvisor_bin_addr = 2333;

  // Print(L"Value at 0x%lx is 0x%lx\n", hvisor_bin_addr,
  //       *(UINTN *)hvisor_bin_addr);

  // Print(L"Value at 0x%lx is 0x%08lx\n", hvisor_bin_start,
  //       *(UINT32 *)hvisor_bin_start);

  Print(L"Copying hvisor binary to 0x%lx, size is %d\n", hvisor_bin_addr,
        hvisor_bin_size);

  CopyMem((void *)hvisor_bin_addr, (void *)hvisor_bin_start, hvisor_bin_size);

  Print(L"Copying hvisor dtb to 0x%lx, size is %d\n", hvisor_dtb_addr,
        hvisor_dtb_size);

  CopyMem((void *)hvisor_dtb_addr, (void *)hvisor_dtb_start, hvisor_dtb_size);

  // dump 20 instructions from hvisor_bin_addr, 4 instructions per line
  Print(L"Dumping instructions from 0x%lx\n", hvisor_bin_addr);
  for (int i = 0; i < 20; i++)
  {
    UINT32 *inst = (UINT32 *)(hvisor_bin_addr + i * 4);
    // 4 instructions per line
    if (i % 4 == 0 && i != 0)
    {
      Print(L"\n");
    }
    Print(L"0x%08x ", *inst);
  }
  Print(L"\n");

  // now dump dtb magic and contents
  Print(L"Dumping dtb after copying\n");
  UINT32 *dtb_magic = (UINT32 *)hvisor_dtb_addr;
  Print(L"dtb magic: 0x%08x\n", *dtb_magic);
  // if not 0xEDFE0DD0, then panic
  if (*dtb_magic != 0xEDFE0DD0)
  {
    Print(L"dtb magic is not 0xEDFE0DD0, panic\n");
    while (1)
    {
    }
  } else {
    Print(L"dtb magic is 0xEDFE0DD0, good\n");
  }

  Print(L"==============================================\n");
  Print(L"Done! now jump to hvisor entry...\n");
  Print(L"==============================================\n\n");

  init_serial();

  //  place dtb addr in r5
  // as constant
  asm volatile("li.d $r5, %0" ::"i"(hvisor_dtb_addr));

  void *hvisor_entry = (void *)hvisor_bin_addr;
  ((void (*)(void))hvisor_entry)();

  while (1)
  {
  }

  return EFI_SUCCESS;
}