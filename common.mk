
TOOLCHAIN_PREFIX := /c/gnuarm/bin/arm-none-eabi-
VERSION := 8.2.1

CC = $(TOOLCHAIN_PREFIX)gcc-$(VERSION)
CXX = $(TOOLCHAIN_PREFIX)g++
AR = $(TOOLCHAIN_PREFIX)ar
AS = $(TOOLCHAIN_PREFIX)as
LD = $(TOOLCHAIN_PREFIX)ld
NM = $(TOOLCHAIN_PREFIX)nm
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP = $(TOOLCHAIN_PREFIX)objdump
RANLIB = $(TOOLCHAIN_PREFIX)ranlib
READELF = $(TOOLCHAIN_PREFIX)readelf
STRIP = $(TOOLCHAIN_PREFIX)strip
SIZE = $(TOOLCHAIN_PREFIX)size

COMPILER_FLAGS = -Os -g -finline \
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
-Werror=return-type \
-ffunction-sections \
-fdata-sections \
-Werror-implicit-function-declaration \
-Wwrite-strings \
-DSTM32F437xx \
-D__thumb__ \
-DENABLE_SWD \
-DUSE_FULL_LL_DRIVER

CFLAGS = --std=gnu11 $(COMPILER_FLAGS) \
-Wstrict-prototypes

CXXFLAGS = --std=c++11 $(COMPILER_FLAGS)

OBJDIR = $(TOPDIR)/.obj

COBJS = $(addprefix $(OBJDIR)/, $(CSRCS:.c=.o))
CXXOBJS = $(addprefix $(OBJDIR)/, $(CXXSRCS:.cpp=.o))
ASOBJS = $(addprefix $(OBJDIR)/, $(ASRCS:.s=.o))

$(COBJS): $(OBJDIR)/%.o: %.c
	@[ -e $(OBJDIR) ] || mkdir -p $(OBJDIR)
#	@[ -e $(OBJDIR)/$(dir $<) ] || mkdir -p $(OBJDIR)/$(dir $<)
	@echo "	CC	$<"
	@$(CC) $(CFLAGS) $(addprefix -I, $(INCDIRS)) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@
	touch -m $(TOPDIR)/target

$(CXXOBJS): $(OBJDIR)/%.o: %.cpp
	@[ -e $(OBJDIR) ] || mkdir -p $(OBJDIR)
#	@[ -e $(OBJDIR)/$(dir $<) ] || mkdir -p $(OBJDIR)/$(dir $<)
	@echo "	CXX	$<"
	@$(CXX) $(CXXFLAGS) $(addprefix -I, $(INCDIRS)) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@
	touch -m $(TOPDIR)/target
	
$(ASOBJS): $(OBJDIR)/%.o: %.s
	@[ -e $(OBJDIR) ] || mkdir -p $(OBJDIR)
#	@[ -e $(OBJDIR)/$(dir $<) ] || mkdir -p $(OBJDIR)/$(dir $<)
	@echo "	AS	$<"
	@$(CC) $(CFLAGS) $(addprefix -I, $(INCDIRS)) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@
	touch -m $(TOPDIR)/target

target_dirs:
	@for d in $(SUBDIRS); do \
		$(MAKE) -s -C $$d -f Makefile target; \
	done

target: $(COBJS) $(CXXOBJS) $(ASOBJS) target_dirs
	@test $(TOPDIR)/target
