ARCH            = loongarch64
QEMU            = qemu-system-loongarch64
CROSS_COMPILE   = loongarch64-unknown-linux-gnu-
CROSS_GDB       = loongarch64-linux-gnu-gdb
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump
READELF         = $(CROSS_COMPILE)readelf
# OBJDUMP		    = rust-objdump
MODE            = debug

CC              = $(CROSS_COMPILE)gcc
CXX             = $(CROSS_COMPILE)g++
LD              = $(CROSS_COMPILE)ld
AR              = $(CROSS_COMPILE)ar

BUILD_DIR       = ./build
DEBUG_DIR       = ./debug
INCLUDE_DIR     = ./include

# Direct .o files to ./build directory
# OBJS            = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(wildcard src/*.c)) $(BUILD_DIR)/kernel.o $(BUILD_DIR)/resources.o $(BUILD_DIR)/switch.o
OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(wildcard src/*.c)) \
       $(patsubst src/%.S,$(BUILD_DIR)/%.o,$(wildcard src/*.S)) 

TARGET          = hvisor.efi
WORKDIR         = $(shell pwd)/./
ELF_TARGET      = $(WORKDIR)/../hvisor/target/loongarch64-unknown-none/debug/hvisor
GNU_EFI         = $(WORKDIR)/gnu-efi-dev-la64
EFIINC          = $(GNU_EFI)/inc
EFIINCS         = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
hvisor_C_INC	= $(WORKDIR)/include
LIB             = $(GNU_EFI)/$(ARCH)/gnuefi
LIB_2           = $(GNU_EFI)/$(ARCH)/lib
RUST_INC        = $(WORKDIR)/../rust_src
RUST_LIB        = $(WORKDIR)/../rust_src/target/$(ARCH)/debug
EFI_CRT_OBJS    = $(LIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(GNU_EFI)/gnuefi/elf_$(ARCH)_efi.lds
CFLAGS          = $(EFIINCS) -I $(RUST_INC) -fno-stack-protector -fpic \
                  -fshort-wchar -Wall -march=loongarch64 -mabi=lp64d -g -Os \
                  -Wextra -fno-strict-aliasing \
                  -ffreestanding -fno-stack-check \
                  $(if $(findstring gcc,$(CC)),-fno-merge-all-constants,) \
                  -I $(WORKDIR)/../extras/font8x8
CFLAGS  				+= -I $(hvisor_C_INC) 

LDFLAGS         = -nostdlib --warn-common --no-undefined -shared \
                  --build-id=sha1 -Bsymbolic --defsym=EFI_SUBSYSTEM=0xa -T $(EFI_LDS) -L $(LIB) -L $(LIB_2) -L $(RUST_LIB) $(EFI_CRT_OBJS) \

LIGHTGREEN_S := \033[1;32m
LIGHTGREEN_E := \033[0m
LIGHTYELLOW_S := \033[1;33m
LIGHTYELLOW_E := \033[0m

F_QEMU ?=

# append QEMU definition as -DQEMU to CFLAGS
ifeq ($(F_QEMU), 1)
	CFLAGS += -DQEMU
endif

all: clean $(TARGET)

hvisor.so: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ -lefi -lgnuefi
# dump elf
	$(OBJDUMP) -x $@ > hvisor.txt
	$(OBJDUMP) -d $@ > hvisor.asm

%.efi: %.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* \
			-j .rela.* -j .rel* -j .rela* -j .reloc -O binary $^ $@
	cp $@ BOOTLOONGARCH64.EFI

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/%.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

create_dir:
	@mkdir -p $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR) $(DEBUG_DIR) *.efi *.so *.txt *.asm
	$(foreach dir,$(GUEST_OS_DIRS),$(MAKE) -C $(GUEST_OS_DIRS_PREFIX)$(dir) clean;)

QEMU_CMD = $(QEMU) -bios QEMU_EFI.fd -kernel $(TARGET) -m 4G -smp 1 -nographic -serial mon:stdio

run: clean $(TARGET)
	$(QEMU_CMD)

run-debug: clean $(TARGET)
	$(QEMU_CMD) -s -S

debug:
	$(CROSS_GDB) -ex "file $(ELF_TARGET)" -ex "target remote localhost:1234" -ex "layout asm"