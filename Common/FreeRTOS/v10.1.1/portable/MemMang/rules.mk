LOC_DIR := $(GET_LOCAL_DIR)

SUBDIRS +=
	
SRCS = \
	$(LOC_DIR)/heap_3.c
	
INCDIRS = \
	$(LOC_DIR)/../../include \
	$(TOPDIR)/Dozer/Main/Header/System \
	$(LOC_DIR)/../GCC/ARM_CM4F

include make/common.mk