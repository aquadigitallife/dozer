LOC_DIR := $(GET_LOCAL_DIR)

SUBDIRS += \
	$(LOC_DIR)/portable/MemMang \
	$(LOC_DIR)/portable/GCC/ARM_CM4F
	
SRCS = \
	$(LOC_DIR)/croutine.c \
	$(LOC_DIR)/list.c \
	$(LOC_DIR)/queue.c \
	$(LOC_DIR)/tasks.c \
	$(LOC_DIR)/timers.c \
	$(LOC_DIR)/stream_buffer.c
	
INCDIRS = \
	$(LOC_DIR)/include \
	$(TOPDIR)/Dozer/Main/Header/System \
	$(LOC_DIR)/portable/GCC/ARM_CM4F

include make/common.mk