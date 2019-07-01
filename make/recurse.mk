SUBDIRS := $(sort $(SUBDIRS))
SUBDIRS := $(filter-out $(ALLSUBDIRS),$(SUBDIRS))

ifneq ($(SUBDIRS),)

ALLSUBDIRS += $(SUBDIRS)
ALLSUBDIRS := $(sort $(ALLSUBDIRS))
RULESDIRS := $(SUBDIRS)
SUBDIRS :=
#$(info including $(RULESDIRS))
include $(addsuffix /rules.mk,$(RULESDIRS))

include make/recurse.mk

endif
