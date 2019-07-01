LOC_DIR := $(GET_LOCAL_DIR)

SUBDIRS +=

SRCS = \
	$(LOC_DIR)/port.c
	
INCDIRS = \
	$(LOC_DIR) \
	$(LOC_DIR)/../../../include \
	$(TOPDIR)/Dozer/Main/Header/System

include make/common.mk