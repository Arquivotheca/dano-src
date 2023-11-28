# CleanTemplate.mk -- Template for 'clean' target
# Copyright (c) 2001; Be, Inc. All Rights Reserved.
#
# Input Parameters: (they have to set before the template inclusion) 
#  *TRASH_FILES		:= the files to throw away
#  *TRASH_ID		:= a unique identifier
#
# Items preceded with an asterisk (*) must exist and can not legally be empty.
#
# Output Parameters: (they become valid after the template inclusion, and are
# invalidated by any other template inclusion)
#
#   TRASH_FILES_OUT := the files that would be trashed if clean were run
#   TRASH_ID_OUT := the processed trash-id
#
# Notes:
#	This template is designed to be included by other templates.

TRASH_FILES_IN := $(strip $(TRASH_FILES))
TRASH_ID_IN := $(strip $(TRASH_ID))

TRASH_FILES_OUT := $(wildcard $(TRASH_FILES_IN))
TRASH_ID_OUT := $(subst $() ,_,$(TRASH_ID_IN))_clean

ifeq ($(TRASH_ID_OUT),_clean)
$(error $(SRC_DIR) - TRASH_ID should be defined to a unique identifier)
endif

ifneq ($(TRASH_FILES_OUT),)

.PHONY: $(TRASH_ID_OUT)

$(TRASH_ID_OUT): TRASHFILES:=$(TRASH_FILES_OUT)

$(TRASH_ID_OUT):
	$(call Prepare, Cleaning data file)
	$(RM) $(TRASHFILES)

clean: $(TRASH_ID_OUT)

endif

TRASH_FILES :=
TRASH_ID :=
