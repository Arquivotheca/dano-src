# GenericGeneratedTemplate.mk -- Generic Generated Template Makefile
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.
#
# Input Parameters: (they have to set before the template inclusion) 
#  *TARGET_NAME		:=	the name of the installed created file
#						The fully qualified target name is passed to MY_RULE
#						as $(1).
#  *MY_RULE			:=	the name of the rule to use -- Remember the rule
#						won't have it's variables expanded when it's defined.
#						Rules should be defined in Macros.mk and end in _RULE.
#	MY_DEPS			:=	the list of fully items to depend upon
#						This list is fully-qualified and passed to MY_RULE as $(2).
#   SRC_DIR			:=	the relative path from $(BUILDHOME) of the root of your
#             	     	application source tree.
#   MY_INSTALL_DIR	:=	the absolute destination directory -- should be an
#						INSTALL_ constant. (defaults to OBJ_DIR)
#						If it defaults to the OBJ_DIR it's treated as a build item
#						and not an installed item. (It's attached to the build vs.
#						install rules.)
#	README				XXX jbq document me
#   ASAP_PAR		:=	Create the file (if needed) when the rule is sourced, instead
#						of when the file is needed.
#	RULE_HELPER_3_PAR := $(3) when MY_RULE is called.
#	RULE_HELPER_4_PAR := $(4) when MY_RULE is called.
#	RULE_HELPER_5_PAR := $(5) when MY_RULE is called.
#	RULE_HELPER_6_PAR := $(6) when MY_RULE is called.
#	RULE_HELPER_7_PAR := $(7) when MY_RULE is called.
#	RULE_HELPER_8_PAR := $(8) when MY_RULE is called.
#	RULE_HELPER_9_PAR := $(9) when MY_RULE is called.
#
# Parameters marked with '*' are required and can not be empty. If you don't
# use something it has to be empty, otherwise Bad Things(tm) will happen.
#
# Output Parameters: (they become valid after the template inclusion, and are
# invalidated by any other template inclusion)
#
#	TARGET		:= The full output name
#	OBJ_DIR		:= the usual object directory
#
#	Todo:

# no recursive inclusion
ifdef INCLUDED_GENERICGENERATEDTEMPLATE_INT
$(error a template cannot include itself (GenericGeneratedTemplate))
endif
INCLUDED_GENERICGENERATEDTEMPLATE_INT=1

TARGET_NAME_IN		:=	$(strip $(TARGET_NAME))
MY_RULE_IN			:=	$(strip $(MY_RULE))
MY_DEPS_IN			:=	$(strip $(MY_DEPS))
SRC_DIR_IN			:=	$(strip $(SRC_DIR))
MY_INSTALL_DIR_IN	:=	$(strip $(MY_INSTALL_DIR))
README_IN			:=	$(strip $(README))
ASAP_IN := $(strip $(ASAP_PAR))
# Note: 0, 1, and 2 are Invalid and will produce an error if used
RULE_HELPER_0_IN := $(strip $(RULE_HELPER_0_PAR))
RULE_HELPER_1_IN := $(strip $(RULE_HELPER_1_PAR))
RULE_HELPER_2_IN := $(strip $(RULE_HELPER_2_PAR))
RULE_HELPER_3_IN := $(strip $(RULE_HELPER_3_PAR))
RULE_HELPER_4_IN := $(strip $(RULE_HELPER_4_PAR))
RULE_HELPER_5_IN := $(strip $(RULE_HELPER_5_PAR))
RULE_HELPER_6_IN := $(strip $(RULE_HELPER_6_PAR))
RULE_HELPER_7_IN := $(strip $(RULE_HELPER_7_PAR))
RULE_HELPER_8_IN := $(strip $(RULE_HELPER_8_PAR))
RULE_HELPER_9_IN := $(strip $(RULE_HELPER_9_PAR))

ifeq ($(MY_RULE_IN),)
$(error MY_RULE undefined or empty. SRC_DIR=$(SRC_DIR_IN) MY_INSTALL_DIR=$(MY_INSTALL_DIR_IN))
endif

ifneq ($(strip $(RULE_HELPER_0_IN)$(RULE_HELPER_1_IN)$(RULE_HELPER_2_IN)),)
$(error RULE_HELPER_x defined where 'x' is less than 3.  RULE_HELPER's are ordered logically instead of linearly. The macro is called with $$(call RULE_NAME, TARGET_NAME, MY_DEPS, RULE_HELPER...) this means $$(3) is the first RULE_HELPER. SRC_DIR=$(SRC_DIR_IN) MY_INSTALL_DIR=$(MY_INSTALL_DIR_IN))
endif

#####################################################################
# Here starts a piece of logic that slightly changes the behavior
# depending on the
# location of the target.
# it also sets the TARGET variable.
#
# if target is to be installed in install tree
#    set TARGET variable
#    set install rules
# else
#    if target is to be installed outside of the install tree (host-based tool)
#       set TARGET variable
#		set build rule
#    else (target is in the object directory, must be cleaned by a make clean)
#       set TARGET variable
#       set build rule
#       set clean rule
#    endif
# endif

ifneq ($(strip $(filter $(INSTALL_BASE)/%, $(MY_INSTALL_DIR))),)
# Target is in the install tree

TARGET:= $(addprefix $(MY_INSTALL_DIR)/,$(TARGET_NAME))

ifdef IGNORE_TARGET_SPEC
$(INSTALL_RULES): $(TARGET)
else
$(INSTALL_RULES): $(filter $(TARGET),$(FILES_SPEC))
FILES_SPEC:= $(filter-out $(TARGET),$(FILES_SPEC))
endif

else

ifneq ($(strip $(MY_INSTALL_DIR)),)
# Target is in the Host Tools directory (or similar)
TARGET:= $(addprefix $(MY_INSTALL_DIR)/,$(TARGET_NAME))

build: $(TARGET)

# When we install to the HOST_TOOLS_BIN directory, we treat it like a
# OBJ_DIR build except we don't erase the TARGET with a 'clean'
else # ($(strip $(MY_INSTALL_DIR)),)
# Target is in the object directory

# If we install in to the OBJ_DIR, we'll erase the file on a 'clean' build.
ifeq ($(SRC_DIR_IN),)
$(error $(TARGET_NAME) - MY_INSTALL_DIR and SRC_DIR can not both be empty)
endif

TARGET:= $(addprefix $(OBJ_DIR_FROM_SRC_DIR)/,$(TARGET_NAME))

build: $(TARGET)

ifneq ($(strip $(TARGET)),)
TRASH_FILES:=$(TARGET)
TRASH_ID:=$(TARGET)
include $(SDIR_TEMPLATES)/CleanTemplate.mk
endif # ($(strip $(TARGET)),)

# end of HOST_TOOLS_BIN vs. OBJ_DIR
endif # ($(strip $(MY_INSTALL_DIR)),)

# end of Install vs. Build
endif

# here ends that piece of logic that differentiates depending
# on the location of the target
#####################################################################

ifneq ($(strip $(TARGET)),)
.DELETE_ON_ERROR: $(TARGET)

$(TARGET): MY_RULE:=$(MY_RULE)

ifneq ($(README_IN),)
# FULL_NAMES properly handles cases where SRC_DIR is empty, however
# in that case, the items in README must be absolute, or relative to BUILDHOME.
$(TARGET): README:=$(call, FULL_NAMES, $(README))
endif

$(TARGET) : $(SDIR_TEMPLATES)/GenericGeneratedTemplate.mk

$(TARGET) : MY_DEPS:=$(call FULL_NAMES, $(MY_DEPS_IN))

$(TARGET) : $(call FULL_NAMES, $(MY_DEPS_IN))
	$(warning *** $@ -- $(MY_DEPS))
	$(call PrepareDir, Creating)
	$(call $(MY_RULE),$@,$(MY_DEPS),$(R3),$(R4),$(R5),$(R6),$(R7),$(R8),$(R9))
	if [ ! -f "$@" ] ; then echo "$@ - Target not created. MY_RULE='$(MY_RULE)'" ; false ; fi
	$(LIST_DEPENDENCIES)

$(TARGET): R3:=$(RULE_HELPER_3_IN)
$(TARGET): R4:=$(RULE_HELPER_4_IN)
$(TARGET): R5:=$(RULE_HELPER_5_IN)
$(TARGET): R6:=$(RULE_HELPER_6_IN)
$(TARGET): R7:=$(RULE_HELPER_7_IN)
$(TARGET): R8:=$(RULE_HELPER_8_IN)
$(TARGET): R9:=$(RULE_HELPER_9_IN)

ifneq ($(ASAP_IN),)

$(GHH_TARGET_OUT): TARGET:=$(TARGET)

$(GHH_TARGET_OUT): MY_DEPS:=$(call FULL_NAMES, $(MY_DEPS_IN))

ASAP_OUT := $(strip $(shell if [ ! -f "$(TARGET)" ] ; then \
	$(call PrepareDir, Creating, $(TARGET)) &> $(MY_PREFIX)/bobs.tmp.log && \
	$(call $(MY_RULE),$(TARGET),$(MY_DEPS)) >> $(MY_PREFIX)/bobs.tmp.log 2>&1 && \
	echo true || echo false ; fi))

ifeq ($(ASAP_OUT),false)
ASAP_INT := $(shell cat $(MY_PREFIX)/bobs.tmp.log 1>&2)
$(error $(TARGET) - Target not created. MY_RULE='$(MY_RULE)')
endif

endif # GENERATED_HEADER_HELPER_IN

endif

# no recursive inclusion
INCLUDED_GENERICGENERATEDTEMPLATE_INT=

TARGET_NAME		:=	
MY_RULE			:=	
MY_DEPS			:=	
SRC_DIR			:=	
MY_INSTALL_DIR	:=	
README			:=
GENERATED_HEADER_HELPER_PAR :=
RULE_HELPER_0_PAR := 
RULE_HELPER_1_PAR := 
RULE_HELPER_2_PAR := 
RULE_HELPER_3_PAR := 
RULE_HELPER_4_PAR := 
RULE_HELPER_5_PAR := 
RULE_HELPER_6_PAR := 
RULE_HELPER_7_PAR := 
RULE_HELPER_8_PAR := 
RULE_HELPER_9_PAR := 
