# SimpleSharedLibTemplate.mk -- Simple Shared Library Template Makefile
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.
#
# Input Parameters: (they have to set before the template inclusion) 
#  *TARGET_NAME		:=	the name of the Shared Library.
#  *MY_INSTALL_DIR	:= 	the full path of the directory you want
#						your app to be installed in. You should use of the
#						globally define INSTALL_* variables define in Init.mk.
#	MY_CFLAGS	:= additional CFLAGS
#	MY_INCLUDES := additional INCLUDES
#	MY_OPTIMIZER :=	the prefered optimizer setting
#  *SRC_DIR		:=	the relative path from $BUILDHOME of the root of the kit
#					source tree
#  *SRCS		:=	the list of source files relative to $(SRC_DIR). 
#   SYSTEM_LIBS	:=	the list of system libraries
#                   that your application links against.
#	LINKS		:=	full path to symlinked name pointing to the installed item.
#					Generally (if used): $(INSTALL_DEV_LIBS)/$(TARGET_NAME)
#   PROFILE_ME  :=  If you should profile this add-on, set this to
#					'true' or '1'. -- False is assumed if empty, '0' or 'false'.
#   UNDEFINED_SYMBOLS := list of symbols which start as undefined.
#					Through the linker's -u or --undefined (in GCC) options.
#	MORE_OBJS	:=	additional objects to link to the target
#					these may be absolute paths and are not cleaned up
#	WHOLE_ARCHIVES := This lists static libraries that you want to completely
#					include in your shared library.
#   OVERRIDE_CPU_FEATURES_DFLAGS := used by CPUFeaturesTemplate.mk
#   OVERRIDE_CPU_ID := used by CPUFeaturesTemplate.mk
#   OVERRIDE_M_CPU  := used by CPUFeaturesTemplate.mk
#   OVERRIDE_M_ARCH := used by CPUFeaturesTemplate.mk
#
# All the input parameters must exist. They should be set empty if they're
# unneeded. Items preceded with an asterisk (*) can not legally be empty.
#
# Output Parameters: (they become valid after the template inclusion, and are
# invalidated by any other template inclusion)
#
#	TARGET		:=	the full path of the generated application
#	LINKS			:=	the names of the symlink files
#   SYSTEM_LIBS_OUT	:=	the full path to the system libs
#	MORE_OBJS_OUT := fully qualified MORE_OBJS list
#
#	Note:
#		Unlike SharedLibTemplate.mk, this template doesn't assume the presence
#		of Kits. -- It's for relatively self-contained shared libraries.
#
#	Todo:

############################################################
# no user-servicable parts beyond this line

# no recursive inclusion
ifdef INCLUDED_SIMPLESHAREDLIBTEMPLATE_INT
$(error a template cannot include itself (SimpleSharedLibTemplate))
endif
INCLUDED_SIMPLESHAREDLIBTEMPLATE_INT=1

TARGET_NAME_IN		:=	$(strip $(TARGET_NAME))
SRC_DIR_IN		:=	$(strip $(SRC_DIR))
SRCS_IN		:=	$(strip $(SRCS))
MY_CFLAGS_IN	:= $(strip $(MY_CFLAGS))
MY_INCLUDES_IN := $(strip $(MY_INCLUDES))
MY_OPTIMIZER_IN :=	$(strip $(MY_OPTIMIZER))
SYSTEM_LIBS_IN	:=	$(strip $(SYSTEM_LIBS))
LINKS_IN		:=	$(strip $(LINKS))
MY_INSTALL_DIR_IN	:= 	$(strip $(MY_INSTALL_DIR))
PROFILE_ME_IN := $(strip $(PROFILE_ME))
OVERRIDE_CPU_FEATURES_DFLAGS_IN := $(strip $(OVERRIDE_CPU_FEATURES_DFLAGS))
OVERRIDE_CPU_ID_IN := $(strip $(OVERRIDE_CPU_ID))
OVERRIDE_M_CPU_IN := $(strip $(OVERRIDE_M_CPU))
OVERRIDE_M_ARCH_IN := $(strip $(OVERRIDE_M_ARCH))
UNDEFINED_SYMBOLS_IN := $(strip $(UNDEFINED_SYMBOLS))
MORE_OBJS_IN := $(strip $(MORE_OBJS))
WHOLE_ARCHIVES_IN := $(strip $(WHOLE_ARCHIVES))
RESOURCE_FORK_IN:=	 $(strip $(RESOURCE_FORK))
RESOURCES_IN := $(strip $(RESOURCES))

LINKS := $(LINKS_IN)

# Save variables that are going to be touched by this template
TEMPLATE_ASFLAGS_SAVE_SIMPLESHAREDLIBTEMPLATE_INT:=$(TEMPLATE_ASFLAGS)
# Don't touch inputs to other templates
OBJ_DIR_SAVE_SIMPLESHAREDLIBTEMPLATE_INT:=$(OBJ_DIR)
TEMPLATE_CFLAGS_SAVE_SIMPLESHAREDLIBTEMPLATE_INT:=$(TEMPLATE_CFLAGS)
SOURCE_SAVE_SIMPLESHAREDLIBTEMPLATE_INT:=$(SOURCE)
LINK_PREFIX_SAVE_SIMPLESHAREDLIBTEMPLATE_INT:=$(LINK_PREFIX)
LINK_RELATION_SAVE_SIMPLESHAREDLIBTEMPLATE_INT:=$(LINK_RELATION)

OBJ_DIR:=$(OBJ_DIR_FROM_SRC_DIR)
ifneq ($(OVERRIDE_CPU_ID),)
OBJ_DIR:=$(addprefix $(OBJ_DIR)/,$(OVERRIDE_CPU_ID))
endif

SRC_DIR_OUT:=$(SRC_DIR_OUT_FROM_SRC_DIR)
SYSTEM_LIBS_OUT:= $(call HANDLE_LIBRARIES, $(SYSTEM_LIBS_IN))
MORE_OBJS_OUT :=  $(call FULL_NAMES, $(MORE_OBJS_IN))
WHOLE_ARCHIVES_OUT := $(call LIBRARIES_OUT, $(WHOLE_ARCHIVES_IN))
RESOURCES_OUT := $(call FULL_NAMES, $(RESOURCE_FORK_IN) $(RESOURCES_IN))
RESOURCE_FORK_OUT = $(error $(SRC_DIR) - $(TARGET_NAME) -- You should be using RESOURCES_OUT instead of RESOURCE_FORK_OUT) 

TARGET:= $(call CREATED_LIBRARY_PATH)/$(TARGET_NAME_IN)
ifneq ($(OVERRIDE_CPU_ID),)
TARGET:= $(patsubst %.so,%, $(TARGET))/$(OVERRIDE_CPU_ID).so
endif

build: $(TARGET)

ifeq ($(OVERRIDE_CPU_ID),default)
build: $(dir $(patsubst %/,%,$(dir $(TARGET))))$(TARGET_NAME_IN)

$(dir $(patsubst %/,%,$(dir $(TARGET))))$(TARGET_NAME_IN): $(TARGET)
	$(call PrepareDir, Creating symlink for build)
	$(MKSYMLINK) $(notdir $(<D))/$(<F) $@
endif

TEMPLATE_CFLAGS := $(SHARED_CFLAGS) $(PUBLIC_INCLUDES)

ifneq ($(OVERRIDE_M_CPU),)
TEMPLATE_CFLAGS := $(filter-out -mcpu=%, $(TEMPLATE_CFLAGS)) $(OVERRIDE_M_CPU)
endif
ifneq ($(OVERRIDE_M_ARCH),)
TEMPLATE_CFLAGS := $(filter-out -march=%, $(TEMPLATE_CFLAGS)) $(OVERRIDE_M_ARCH)
endif

TEMPLATE_ASFLAGS := $(SHARED_ASFLAGS)

ifeq ($(CHECK_PROFILING),1)
	TEMPLATE_CFLAGS += $(PROFILE_FLAG)
	SYSTEM_LIBS_OUT += $(PROFILE_GLUE)
endif

$(TARGET): $(BEGIN_GLUE) 

# TARGET := $(TARGET)
# SRC_DIR := $(SRC_DIR)
# SRCS := $(SRCS)
# OBJ_DIR := $(OBJ_DIR)
# MY_CFLAGS := $(MY_CFLAGS)
# MY_INCLUDES := $(MY_INCLUDES)
# MY_OPTIMIZER := $(MY_OPTIMIZER)
include $(SDIR_TEMPLATES)/CompileTemplate.mk
TARGET := $(TARGET_IN)

ifneq ($(OVERRIDE_CPU_FEATURES_DFLAGS_IN),)
$(MORE_OBJS_OUT) $(OBJS_OUT): DFLAGS_SPEC := $(filter-out -D_PROCESSOR_%,$(DFLAGS_SPEC)) $(OVERRIDE_CPU_FEATURES_DFLAGS_IN)
endif

# Note: If you even /think/ about "simplifying" this by having the following
#  prerequisite in the same place as the link rule, it will break.
#
#  Apparently prerequisites added with the rule are placed at the *beginning*
#  of the list. In cases like this were the glue must be first, this breaks
#  things. -- This seems to only really be problematic when a static library
#  is in SYSTEM_LIBS -- which is fairly common for drivers. (steven 00-07-19)
$(TARGET): $(MORE_OBJS_OUT) $(END_GLUE) $(WHOLE_ARCHIVES_OUT) $(SYSTEM_LIBS_OUT)
$(TARGET): $(RESOURCES_OUT)

$(TARGET): LDFLAGS:= $(subst -Xlinker -x,,$(SHARED_LDFLAGS))
ifneq ($(OVERRIDE_CPU_ID),)
$(TARGET): SET_SONAME:=-Xlinker -soname=_CPU_$(patsubst %.so,%,$(TARGET_NAME))
else
$(TARGET): SET_SONAME:=-Xlinker -soname=$(TARGET_NAME)
endif

$(TARGET): UNDEFINED_SYMBOLS:=$(addprefix -u ,$(UNDEFINED_SYMBOLS_IN))
$(TARGET): WHOLE_ARCHIVES_OUT:=$(WHOLE_ARCHIVES_OUT)

.DELETE_ON_ERROR: $(TARGET)
$(TARGET): 
	$(call PrepareDir, Linking)
	$(LD) $(LDFLAGS) -o "$@" $(SET_SONAME) $(UNDEFINED_SYMBOLS) \
		$(call LINKABLE_ITEMS, $+, $(WHOLE_ARCHIVES_OUT)) \
		$(call MAKE_WHOLE_ARCHIVES_REFERENCE, $(WHOLE_ARCHIVES_OUT)) \
		$(WRITE_REPORT)
ifneq ($(call MFLAG_IS_SET, STRIP_BINARIES),)
	-$(STRIP)
endif
ifneq ($(RESOURCES_OUT),)
	-$(call DO_RESOURCES)
endif
ifneq ($(call MFLAG_IS_SET, DO_SETVERSION),)
	-$(DO_SETVERSION)
endif
	-$(MIMESET) -f "$@"
	$(LIST_DEPENDENCIES)

ifneq ($(MY_INSTALL_DIR_IN),)

MY_INSTALL_OUT := $(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN)

ifneq ($(OVERRIDE_CPU_ID),)
MY_INSTALL_OUT := $(patsubst %.so,%, $(MY_INSTALL_OUT))/$(OVERRIDE_CPU_ID).so
endif

ifeq ($(OVERRIDE_CPU_ID),default)
$(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN): $(MY_INSTALL_OUT)
	$(call PrepareDir, Installing)
	$(MKSYMLINK) $(notdir $(<D))/$(<F) $@
endif

$(MY_INSTALL_OUT): $(INSTALL_FILE_DEPENDANCIES)

$(MY_INSTALL_OUT): $(TARGET)
	$(DO_INSTALL_FILE)

ELF_FILES+=$(MY_INSTALL_OUT)

ifeq ($(OVERRIDE_CPU_ID),default)
ifdef IGNORE_TARGET_SPEC
$(INSTALL_RULES): $(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN)
else
$(INSTALL_RULES): $(filter $(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN),$(FILES_SPEC))
FILES_SPEC:= $(filter-out $(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN),$(FILES_SPEC))
endif
endif

ifdef IGNORE_TARGET_SPEC
$(INSTALL_RULES): $(MY_INSTALL_OUT)
buildlibs: $(TARGET)
else
$(INSTALL_RULES): $(filter $(MY_INSTALL_OUT),$(FILES_SPEC))
buildlibs: $(if $(filter $(MY_INSTALL_OUT),$(FILES_SPEC)),$(TARGET),)
FILES_SPEC:= $(filter-out $(MY_INSTALL_OUT),$(FILES_SPEC))
endif

ifeq ($(if $(OVERRIDE_CPU_ID_IN),$(OVERRIDE_CPU_ID_IN),default),default)

ifneq ($(call MFLAG_IS_SET, DEVELOP_LIBS_ARE_FILES),)

LINKS_ARE_FILES_INT := $(filter $(INSTALL_DEV_LIBS)/%,$(LINKS))

ifneq ($(LINKS_ARE_FILES_INT),)

SOURCE        := $(TARGET)
SRC_DIR       := $(SRC_DIR_IN)
TARGET        := $(word 1, $(LINKS_ARE_FILES_INT))
ATTRIBUTES    := 
PERMISSIONS   := 
MY_DEPS := $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk
include $(SDIR_TEMPLATES)/DataFileTemplate.mk	

ELF_FILES += $(TARGET_IN)

LINKS := $(filter-out $(TARGET_IN), $(LINKS))
TARGET_IN := $(SOURCE_IN)

endif
endif

# TARGET_IN should always be a file in the install tree.
SOURCE:=/boot/beos/system/lib/$(notdir $(MY_INSTALL_OUT))
SRC_DIR := $(SRC_DIR_IN)
LINK_PREFIX:=
# LINKS := $(LINKS_IN)
ifneq ($(strip $(LINKS)),)
LINK_RELATION:=
#LINK_RELATION:=$(if $(filter-out $(MY_INSTALL_DIR_IN)/%,$(LINKS)),$(INSTALL_BASE),$(MY_INSTALL_DIR_IN))
MY_DEPS := $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk
MY_DEPS_IN = $(error SimpleSharedLibTemplate stomped on expected value.)
endif
endif
else
$(error ($(TARGET_NAME_IN)) MY_INSTALL_DIR needs to be defined for shared libraries.)
endif


# restore variables
TEMPLATE_ASFLAGS:=$(TEMPLATE_ASFLAGS_SAVE_SIMPLESHAREDLIBTEMPLATE_INT)
# Don't touch inputs to other templates
OBJ_DIR:=$(OBJ_DIR_SAVE_SIMPLESHAREDLIBTEMPLATE_INT)
TEMPLATE_CFLAGS:=$(TEMPLATE_CFLAGS_SAVE_SIMPLESHAREDLIBTEMPLATE_INT)
SOURCE:=$(SOURCE_SAVE_SIMPLESHAREDLIBTEMPLATE_INT)
LINK_PREFIX:=$(LINK_PREFIX_SAVE_SIMPLESHAREDLIBTEMPLATE_INT)
LINK_RELATION:=$(LINK_RELATION_SAVE_SIMPLESHAREDLIBTEMPLATE_INT)

# no recursive inclusion
INCLUDED_SIMPLESHAREDLIBTEMPLATE_INT=

TARGET_NAME		:=	
SRC_DIR		:=	
SRCS		:=	
MY_CFLAGS	:= 
MY_INCLUDES := 
MY_OPTIMIZER :=	
SYSTEM_LIBS	:=	
LINKS		:=	
MY_INSTALL_DIR	:= 	
PROFILE_ME :=
OVERRIDE_CPU_FEATURES_DFLAGS :=
OVERRIDE_CPU_ID :=
OVERRIDE_M_CPU :=
OVERRIDE_M_ARCH :=
UNDEFINED_SYMBOLS :=
MORE_OBJS :=
WHOLE_ARCHIVES :=
RESOURCE_FORK :=
RESOURCES :=
