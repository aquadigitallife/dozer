LOC_DIR := $(GET_LOCAL_DIR)

SUBDIRS +=

SRCS = \
	$(LOC_DIR)/rtc.cpp
	
INCDIRS = \
	$(LOC_DIR)/../../../Header \
	$(LOC_DIR)/../../../Header/Global \
	$(LOC_DIR)/../../../Header/System \
	$(LOC_DIR)/../../../Header/BLE \
	$(LOC_DIR)/../../../Header/RTC \
	$(TOPDIR)/Common/Lib \
	$(TOPDIR)/Common/Lib/CRC \
	$(TOPDIR)/Common/Lib/cJSON \
	$(TOPDIR)/Common/CMSIS \
	$(TOPDIR)/Common/STM32F4xx_HAL_Driver/Inc \
	$(TOPDIR)/Common/Periphery/EFlash \
	$(TOPDIR)/Common/Periphery/Motors \
	$(TOPDIR)/Common/Periphery/I2C \
	$(TOPDIR)/Common/Periphery/SIM5320 \
	$(TOPDIR)/Common/Device/Flash \
	$(TOPDIR)/Common/FreeRTOS/v10.1.1/include \
	$(TOPDIR)/Common/FreeRTOS/v10.1.1/portable/GCC/ARM_CM4F

include make/common.mk