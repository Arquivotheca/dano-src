# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.
#
# still an early prototype. template to create a folder
#

# no recursive inclusion
ifdef INCLUDED_FOLDERTEMPLATE_INT
$(error a template cannot include itself (FolderTemplate))
endif
INCLUDED_FOLDERTEMPLATE_INT=1

DIR_NAMES_IN := $(strip $(DIR_NAMES))

ifdef NO_THIS_WAS_NOT_DEFINED
ifdef IGNORE_TARGET_SPEC
STAMP_FILES:=$(patsubst $(INSTALL_BASE)/%,$(STAMP_INSTALL_BASE)/%_dir_stamp,$(DIR_NAMES))
$(INSTALL_RULES): $(STAMP_FILES)
else
STAMP_FILES:= $(patsubst $(INSTALL_BASE)/%,$(STAMP_INSTALL_BASE)/%_dir_stamp,$(filter $(DIR_NAMES),$(FILES_SPEC)))
$(INSTALL_RULES): $(STAMP_FILES)
FILES_SPEC:= $(filter-out $(DIR_NAMES),$(FILES_SPEC))
endif

$(STAMP_FILES) : % :
	$(MKDIR) $(patsubst $(STAMP_INSTALL_BASE)/%_dir_stamp,$(INSTALL_BASE)/%,$@)
	$(MKDIR) $(dir $@)
	$(TOUCH) $@
endif

ifneq ($(DIR_NAMES_IN),)
ifneq ($(filter $(DIR_NAMES_IN), $(FILES_PLAIN_SPEC) $(FILES_ELF_SPEC) $(FILES_LINK_SPEC)),)
$(error The following files are DIRSs in the makefile but defined as something else in the spec file: \
$(filter $(DIR_NAMES_IN), $(FILES_PLAIN_SPEC) $(FILES_ELF_SPEC) $(FILES_LINK_SPEC)))
endif
endif

ifdef IGNORE_TARGET_SPEC
$(INSTALL_RULES): $(DIR_NAMES)
else
$(INSTALL_RULES): $(filter $(DIR_NAMES),$(FILES_SPEC))
FILES_SPEC:= $(filter-out $(DIR_NAMES),$(FILES_SPEC))
endif

## The following is the logic normally used in these cases.
## It isn't needed here, as if the directory exists at all, make won't attempt to update it.
#	$(if $(wildcard $(call ESCAPE_SPACE, $@)),$(MKDIR) "$@",)

$(DIR_NAMES) : % :
	$(MKDIR) "$@"

# no recursive inclusion
INCLUDED_FOLDERTEMPLATE_INT=

DIR_NAMES :=
