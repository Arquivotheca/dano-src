# SymlinkTemplate.mk -- Symlink Template
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.
#
# Input Parameters: (they have to set before the template inclusion) 
#  *SOURCE        := linked-to file
#	SRC_DIR		  := maintained for compatibility
#   LINK_PREFIX   := the base directory for LINKS (May be empty)
#					 (totally optional) (equilivant to MY_INSTALL_DIR)
#   LINK_RELATION := the common root dir of SOURCE and LINK_PREFIX
#                    if this is empty, you get an absolute link, otherwise
#                    relative (required for relative links)
#   LINKS         := the links to create, based on LINK_PREFIX if available
#	MY_DEPS :=	additional dependancies
#
# All the input parameters must exist. They should be set empty if they're
# unneeded. Items preceded with an asterisk (*) can not legally be empty.
#
# Output Parameters: (they become valid after the template inclusion, and are
# invalidated by any other template inclusion)
#
#   LINKS_OUT    := the value of LINKS with the (optional) LINK_PREFIX added
#
# Note: This file is designed to be included by other template files.
#   As such, it *can not* include Make.implicit, or whereever it is that the
#   'clean' rule is defined. It also can't over-write any variables common to
#   other makefile templates, such as TARGET, SRCS, etc.
#
# Note: This makefile must take in to the account that LINKS may be null. That
#   is not a bug. This is due to the fact that the template which includes
#   this is not required to check to see if it needs to create any links.

# Note: MY_DEPS support was pulled (yet the handling of input
#  and output parameters was kept for potential compatibility
#  issues.) as any time you need MY_DEPS, you need to find
#  another solution. You can't depend upon a symlink because
#  their timestamps don't appear to change when the linked-to
#  file is updated.

# no recursive inclusion
ifdef INCLUDED_SYMLINKTEMPLATE_INT
$(error a template cannot include itself (SymlinkTemplate))
endif
INCLUDED_SYMLINKTEMPLATE_INT=1

SOURCE_IN        := $(strip $(SOURCE))
SRC_DIR_IN		:= $(strip $(SRC_DIR))
LINK_PREFIX_IN   := $(strip $(LINK_PREFIX))
LINK_RELATION_IN := $(strip $(patsubst %/, %, $(LINK_RELATION)))
LINKS_IN         := $(strip $(LINKS))
MY_DEPS_IN		:= $(strip $(MY_DEPS))

# SRC_DIR isn't used to straighten-up SOURCE, because nothing is
# to be created inside the source tree.

LINKS_OUT := $(call FULL_NAMES, $(LINKS), $(LINK_PREFIX))
MY_DEPS_OUT := $(call FULL_NAMES, $(MY_DEPS_IN))
# $(SDIR_TEMPLATES)/SymlinkTemplate.mk

ifneq ($(LINKS_OUT),)
ifneq ($(filter $(LINKS_OUT), $(FILES_PLAIN_SPEC) $(FILES_ELF_SPEC) $(FILES_DIR_SPEC)),)
$(error The following files are LINKs in the makefile but defined as something else in the spec file: \
$(filter $(LINKS_OUT), $(FILES_PLAIN_SPEC) $(FILES_ELF_SPEC) $(FILES_DIR_SPEC)))
endif
endif

ifeq ($(strip $(filter-out $(MAKEFILE_LEAFNAME), $(notdir $(filter-out $(SDIR_TEMPLATES)/%, $(MY_DEPS_OUT))))),)
ifeq ($(MY_DEPS_OUT),)
$(warning $(SOURCE) -- $(SRC_DIR_IN) -- Please add $$(MAKEFILE_LEAFNAME) to the MY_DEPS.)
endif
endif

# Don't freak if we have nothing to do
ifneq ($(strip $(LINKS_OUT)),)

$(LINKS_OUT): SOURCE:=$(SOURCE) 

ifeq ($(strip $(LINK_RELATION_IN)),)
$(LINKS_OUT) :
	$(RM) "$@"
	$(call PrepareDir, Symlinking) 
	$(MKSYMLINK) $(SOURCE) $@
else

$(LINKS_OUT): LINK_RELATION:=$(LINK_RELATION_IN)

$(LINKS_OUT) : $(MY_DEPS_OUT)

$(LINKS_OUT) : $(MY_DEPS_OUT)

$(LINKS_OUT) :
	$(call PrepareDir, Symlinking)
	$(RM) "$@"
	$(MKSYMLINK) $(addsuffix /,$(subst $() ,/,$(foreach SymlinkTemp,$(subst /, ,$(subst \
		$() ,_,$(subst $(LINK_RELATION)/,,$(dir $@)))),..)))$(subst \
		$(LINK_RELATION)/,,$(SOURCE)) $@
endif

# $(SOURCE)_clean is probably not unique, so we can't add anything to it.

#.PHONY: $(SOURCE)_clean
#$(SOURCE)_clean: $(SOURCE)_cleanlinks

#$(SOURCE)_cleanlinks: LINKS_OUT:=$(LINKS_OUT)

# Why would you want to clean the install tree ?
# If you have bad/invalid/obsolete links installed in the install tree. (steven)
# I think it should be a pseudo-target other than 'clean' (steven)

#$(SOURCE)_cleanlinks:
#	$(call Prepare, Cleaning links, $(word 1, $(LINKS_OUT))...)
#	rm -rf $(LINKS_OUT)

ifdef IGNORE_TARGET_SPEC
$(INSTALL_RULES): $(LINKS_OUT)
else
$(INSTALL_RULES): $(filter $(LINKS_OUT),$(FILES_SPEC))
FILES_SPEC:= $(filter-out $(LINKS_OUT),$(FILES_SPEC))
endif

#clean: $(SOURCE)_clean

endif

# no recursive inclusion
INCLUDED_SYMLINKTEMPLATE_INT=

SOURCE        := 
LINK_PREFIX   := 
LINK_RELATION := 
LINKS         := 
MY_DEPS :=
SRC_DIR :=
