TOPDIR = .

SUBDIRS = \
	Common/CMSIS \
	Common/FreeRTOS/V9.0.0/Portable/GCC/ARM_CM4F \
	Common/FreeRTOS/V9.0.0/Source \
	Common/Lib/Tools \
	Common/Lib/CRC \
	Common/Periphery/EFlash \
	Common/System \
	Dozer/Main/Source
	
CSRCS =

CXXSRCS =

GLOBAL_LDFLAGS = -z max-page-size=4096 -nostartfiles --gc-sections

LINKER_SCRIPT = stm32_flash.ld

dozer: target
	@echo "	LD	$@"
	@$(SIZE) -t --common $(sort $(wildcard .obj/*.o))
	@$(LD) $(GLOBAL_LDFLAGS) -T $(LINKER_SCRIPT) $(wildcard .obj/*.o) -L$(LIBCDIR) -L$(LIBGCCDIR) -lc -lgcc -Map=$@.map -o $@.elf

all: dozer

clean:
	@rm -f .obj/* *.elf

distclean: clean
	@rm -rf .obj *.map

include $(TOPDIR)/common.mk

LIBGCCDIR := $(dir $(shell $(CC) $(CFLAGS) -print-libgcc-file-name))
LIBCDIR := $(dir $(shell $(CC) $(CFLAGS) -print-file-name=libc.a))