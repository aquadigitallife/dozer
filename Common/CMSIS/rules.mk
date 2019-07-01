LOC_DIR := $(GET_LOCAL_DIR)

SUBDIRS +=

SRCS = \
	$(LOC_DIR)/system_stm32f4xx.c \
	$(LOC_DIR)/startup_stm32f437xx.s

include make/common.mk