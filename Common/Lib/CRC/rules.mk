LOC_DIR := $(GET_LOCAL_DIR)

SUBDIRS +=

SRCS = \
	$(LOC_DIR)/CRC.cpp

INCDIRS = \
	$(LOC_DIR) \
	$(LOC_DIR)/.. \
	$(LOC_DIR)/../cJSON \
	$(TOPDIR)/Dozer/Main/Header \
	$(TOPDIR)/Dozer/Main/Header/Global \
	$(TOPDIR)/Dozer/Main/Header/BLE \
	$(TOPDIR)/Dozer/Main/Header/System \
	$(LOC_DIR)/../../CMSIS \
	$(LOC_DIR)/../../STM32F4xx_HAL_Driver/Inc \
	$(LOC_DIR)/../../Periphery/EFlash \
	$(LOC_DIR)/../../Periphery/Motors \
	$(LOC_DIR)/../../Periphery/SIM5320 \
	$(LOC_DIR)/../../Device/Flash \
	$(LOC_DIR)/../../FreeRTOS/v10.1.1/include \
	$(LOC_DIR)/../../FreeRTOS/v10.1.1/portable/GCC/ARM_CM4F

include make/common.mk