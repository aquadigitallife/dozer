TOPDIR = .

SUBDIRS = Common/CMSIS
SUBDIRS += Common/STM32F4xx_HAL_Driver/Src
SUBDIRS += Common/FreeRTOS/V10.1.1
SUBDIRS += Common/Lib/Tools
SUBDIRS += Common/Lib/CRC
SUBDIRS += Common/Lib/cJSON
SUBDIRS += Common/Periphery/EFlash
SUBDIRS += Common/Periphery/Motors
SUBDIRS += Common/Periphery/UART
SUBDIRS += Common/Periphery/I2C
SUBDIRS += Common/Periphery/SPI
SUBDIRS += Common/Periphery/SIM5320
SUBDIRS += Common/System
SUBDIRS += Dozer/Main/Source
	
CSRCS =

CXXSRCS =

GLOBAL_LDFLAGS = -z max-page-size=4096 -nostartfiles --gc-sections

LINKER_SCRIPT = STM32F437GTx_FLASH.ld

OPENOCD_DIR = /c/openocd
OPENOCD = $(OPENOCD_DIR)/bin/openocd
OPENOCD_SCRIPTS_DIR = $(OPENOCD_DIR)/share/openocd/scripts

dozer.elf: target
	@echo "	LD	$@"
	@$(SIZE) -t --common $(sort $(wildcard .obj/*.o))
	@$(LD) $(GLOBAL_LDFLAGS) -T $(LINKER_SCRIPT) $(wildcard .obj/*.o) -L$(LIBCDIR) -L$(LIBGCCDIR) -lc -lgcc -Map=$(basename $@).map -o $@

debug: dozer.elf
	@$(OPENOCD) -s $(OPENOCD_SCRIPTS_DIR) -f ./openocd/stm32f4_stlinkv2_mini.cfg -c init -c "reset halt" &
	sleep 3
	$(GDB) -q -ex 'target remote localhost:3333' $<
#	$(shell kill $(ps | grep openocd | sed 's/\s\+/ /g' | cut -d' ' -f2))

upload: dozer.elf
	@$(OBJCOPY) -O binary $< $(basename $<).bin
	@$(OPENOCD) -s $(OPENOCD_SCRIPTS_DIR) -f ./openocd/stm32f4_stlinkv2_mini.cfg -c init -c "reset halt;flash probe 0;stm32f2x mass_erase 0; sleep 500;flash write_bank 0 $(basename $<).bin;sleep 500;flash verify_bank 0 $(basename $<).bin 0;reset run;exit"
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
	@rm -rf .obj/* *.elf *.bin *.map target

distclean: clean
	@rm -rf .obj

include $(TOPDIR)/common.mk

LIBGCCDIR := $(dir $(shell $(CC) $(CFLAGS) -print-libgcc-file-name))
LIBCDIR := $(dir $(shell $(CC) $(CFLAGS) -print-file-name=libc.a))