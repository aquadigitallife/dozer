TOPDIR = .

SUBDIRS = Common/CMSIS \
	Common/STM32F4xx_HAL_Driver/Src \
	Common/FreeRTOS/V10.1.1 \
	Common/Lib/Tools \
	Common/Lib/CRC \
	Common/Lib/cJSON \
	Common/Periphery/EFlash \
	Common/Periphery/Motors \
	Common/Periphery/UART \
	Common/Periphery/I2C \
	Common/Periphery/SPI \
	Common/Periphery/SIM5320 \
	Common/System \
	Dozer/Main/Source
	
include make/toolchain.mk

OBJDIR = $(TOPDIR)/.obj
LINKER_SCRIPT = STM32F437GTx_FLASH.ld

OPENOCD_DIR = /c/openocd
OPENOCD = $(OPENOCD_DIR)/bin/openocd
OPENOCD_SCRIPTS_DIR = $(OPENOCD_DIR)/share/openocd/scripts

all: dozer.elf

include make/recurse.mk

dozer.elf: $(OBJS)
	@echo "	LD	$@"
	@$(SIZE) -t --common $(sort $^)
	@$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) $^ -L$(LIBCDIR) -L$(LIBGCCDIR) -lc -lgcc -Map=$(basename $@).map -o $@
	
dozer.bin: dozer.elf
	@$(OBJCOPY) -O binary $< $(basename $<).bin

debug: dozer.elf
	@$(OPENOCD) -s $(OPENOCD_SCRIPTS_DIR) -f ./openocd/stm32f4_stlinkv2_mini.cfg -c init -c "reset halt" &
	sleep 3
	$(GDB) -q -ex 'target remote localhost:3333' $<

upload: dozer.bin
#	@$(OBJCOPY) -O binary $< $(basename $<).bin
	@$(OPENOCD) -s $(OPENOCD_SCRIPTS_DIR) -f ./openocd/stm32f4_stlinkv2_mini.cfg -c init -c "reset halt;flash probe 0;stm32f2x mass_erase 0; sleep 500;flash write_bank 0 $<;sleep 500;flash verify_bank 0 $< 0;reset run;exit"
#	@rm -f $<

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


clean:
	@rm -rf $(OBJDIR)/* *.elf *.map

distclean: clean
	@rm -rf $(OBJDIR)

