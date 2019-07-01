
CSRCS := $(filter %.c,$(SRCS))
CPPSRCS := $(filter %.cpp,$(SRCS))
ASMSRCS := $(filter %.s,$(SRCS))


COBJS = $(addprefix $(OBJDIR)/, $(CSRCS:.c=.o))
CPPOBJS = $(addprefix $(OBJDIR)/, $(CPPSRCS:.cpp=.o))
ASMOBJS = $(addprefix $(OBJDIR)/, $(ASMSRCS:.s=.o))

LOCOBJS = $(COBJS) $(CPPOBJS) $(ASMOBJS)

$(LOCOBJS): INCDIRS := $(INCDIRS)

$(COBJS): $(OBJDIR)/%.o: %.c
	@[ -e $(OBJDIR)/$(dir $<) ] || mkdir -p $(OBJDIR)/$(dir $<)
	@echo "	CC	$(notdir $<)"
	@$(CC) $(CFLAGS) $(addprefix -I, $(INCDIRS)) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@

$(CPPOBJS): $(OBJDIR)/%.o: %.cpp
	@[ -e $(OBJDIR)/$(dir $<) ] || mkdir -p $(OBJDIR)/$(dir $<)
	@echo "	CXX	$(notdir $<)"
	@$(CXX) $(CXXFLAGS) $(addprefix -I, $(INCDIRS)) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@
	
$(ASMOBJS): $(OBJDIR)/%.o: %.s
	@[ -e $(OBJDIR)/$(dir $<) ] || mkdir -p $(OBJDIR)/$(dir $<)
	@echo "	AS	$(notdir $<)"
	@$(CC) $(CFLAGS) $(addprefix -I, $(INCDIRS)) -c $< -MD -MP -MT $@ -MF $(@:%o=%d) -o $@
	
OBJS := $(OBJS) $(LOCOBJS)



