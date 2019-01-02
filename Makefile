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

LINKER_SCRIPT = STM32F437GTx_FLASH.ld

OPENOCD_DIR = /c/OpenOCD-20170821
OPENOCD = $(OPENOCD_DIR)/bin/openocd
OPENOCD_SCRIPTS_DIR = $(OPENOCD_DIR)/share/openocd/scripts

dozer.elf: target
	@echo "	LD	$@"
	@$(SIZE) -t --common $(sort $(wildcard .obj/*.o))
	@$(LD) $(GLOBAL_LDFLAGS) -T $(LINKER_SCRIPT) $(wildcard .obj/*.o) -L$(LIBCDIR) -L$(LIBGCCDIR) -lc -lgcc -Map=$(basename $@).map -o $@

upload: dozer.elf
	@$(OBJCOPY) -O binary $< $(basename $<).bin
	@$(OPENOCD) -s $(OPENOCD_SCRIPTS_DIR) -f ./openocd/stm32f4_stlinkv2_mini.cfg -c init -c "reset halt;flash probe 0;stm32f2x mass_erase 0; sleep 500;flash write_bank 0 $(basename $<).bin;sleep 500;reset run;exit"
	@rm -f $(basename $<).bin

download:
	@if [ $(OUTFILE) ]; then \
	$(OPENOCD) -s $(OPENOCD_SCRIPTS_DIR) -f ./openocd/stm32f4_stlinkv2_mini.cfg -c init -c "flash read_bank 0 $(OUTFILE);exit"; \
	else \
	$(OPENOCD) -s $(OPENOCD_SCRIPTS_DIR) -f ./openocd/stm32f4_stlinkv2_mini.cfg -c init -c "flash read_bank 0 readed.bin;exit"; \
	fi

flash:
	@if [ $(INFILE) ]; then \
	$(OPENOCD) -s $(OPENOCD_SCRIPTS_DIR) -f ./openocd/stm32f4_stlinkv2_mini.cfg -c init -c "reset halt;flash probe 0;stm32f2x mass_erase 0; sleep 500;flash write_bank 0 $(INFILE);sleep 500;reset run;exit"; \
	else \
	echo "Please type: make INFILE=<file to flash> flash"; \
	fi
	
ocd_reset:
	$(OPENOCD) -s $(OPENOCD_SCRIPTS_DIR) -f ./openocd/stm32f4_stlinkv2_mini.cfg -c init -c "reset run" -c exit

all: dozer.elf

clean:
	@rm -f .obj/* *.elf *.bin *.map

distclean: clean
	@rm -rf .obj

include $(TOPDIR)/common.mk

LIBGCCDIR := $(dir $(shell $(CC) $(CFLAGS) -print-libgcc-file-name))
LIBCDIR := $(dir $(shell $(CC) $(CFLAGS) -print-file-name=libc.a))