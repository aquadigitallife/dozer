LOC_DIR := $(GET_LOCAL_DIR)

SUBDIRS +=

SRCS = \
	$(LOC_DIR)/stm32f4xx_ll_gpio.c \
	$(LOC_DIR)/stm32f4xx_ll_rcc.c
#	$(LOC_DIR)/stm32f4xx_ll_usart.c
#	$(LOC_DIR)/stm32f4xx_ll_dma.c
	

INCDIRS = \
	$(LOC_DIR)/../Inc \
	$(LOC_DIR)/../../CMSIS

include make/common.mk