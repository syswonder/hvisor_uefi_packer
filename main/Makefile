obj-y := main.o
obj-y += data.o core.o acpi.o parse.o

$(info ARCH is $(ARCH))

# if ARCH=loongarch64, include ./arch/loongarch64/Makefile
ifeq ($(ARCH),loongarch64)
include main/arch/loongarch64/Makefile
endif
