
TOOLCHAIN_PREFIX := /c/gnuarm/bin/arm-none-eabi-
VERSION := 8.2.1

CC = $(TOOLCHAIN_PREFIX)gcc
CXX = $(TOOLCHAIN_PREFIX)g++
AR = $(TOOLCHAIN_PREFIX)ar
AS = $(TOOLCHAIN_PREFIX)as
LD = $(TOOLCHAIN_PREFIX)ld
GDB = $(TOOLCHAIN_PREFIX)gdb
NM = $(TOOLCHAIN_PREFIX)nm
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP = $(TOOLCHAIN_PREFIX)objdump
RANLIB = $(TOOLCHAIN_PREFIX)ranlib
READELF = $(TOOLCHAIN_PREFIX)readelf
STRIP = $(TOOLCHAIN_PREFIX)strip
SIZE = $(TOOLCHAIN_PREFIX)size

COMPILER_FLAGS = -O0 -g -finline \
-mcpu=cortex-m4 \
-mthumb \
-mthumb-interwork \
-mfpu=fpv4-sp-d16 \
-mfloat-abi=hard \
--specs=nano.specs \
--specs=nosys.specs \
-W -Wall \
-Wno-multichar \
-Wno-unused-parameter \
-Wno-unused-function \
-Wno-unused-label \
-Wno-type-limits \
-Wno-deprecated-declarations \
-Werror=return-type \
-ffunction-sections \
-fdata-sections \
-Werror-implicit-function-declaration \
-Wwrite-strings \
-DSTM32F437xx \
-D__thumb__ \
-DENABLE_SWD \
-DUSE_FULL_LL_DRIVER

CFLAGS = --std=gnu11 $(COMPILER_FLAGS)
# -Wstrict-prototypes

CXXFLAGS = --std=c++11 $(COMPILER_FLAGS)

LDFLAGS = -z max-page-size=4096 -nostartfiles --gc-sections

LIBGCCDIR := $(dir $(shell $(CC) $(CFLAGS) -print-libgcc-file-name))
LIBCDIR := $(dir $(shell $(CC) $(CFLAGS) -print-file-name=libc.a))


GET_LOCAL_DIR = $(patsubst %/,%,$(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))
