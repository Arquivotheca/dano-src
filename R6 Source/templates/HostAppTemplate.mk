# HostAppTemplate.mk -- Application Template
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.
#
# Input Parameters: (they have to set before the template inclusion) 
#  *TARGET_NAME:=	the name of the Executable.
#  *SRC_DIR:=		the relative path to your sources
#  *SRCS:=			the list of source files relative to $(SRC_DIR).
#	MY_CFLAGS:=		Template User provided flags.
#   MY_INCLUDES:=	Additional headers (as might be appended to CFLAGS)
#	MY_OPTIMIZER :=	the prefered optimizer setting
#	SYSTEM_LIBS:=	the list of system libraries
#                   that your application links against.
#	RESOURCES:=	the file(s) containing the resources you want to add to your app.
#                   Its suffix has to be '.rsrc'. RESOURCE_FORK is a synonym
#   MORE_OBJS       := This is a list of absolute or relative
#                      (to SRC_DIR) paths for additional objs
#                      (for example objs checked in to the tree.)
#   NEEDS_DFLAGS := specifies whether we want the DFLAGS, or whether we should
#					clear them out. (default is to keep them)
#					** This parameter isn't available yet. **
#	MY_DEPS :=	additional dependancies
#
# Output Parameters: (they become valid after the template inclusion, and are
# invalidated by any other template inclusion)
#
#	SRC_DIR_OUT:=	the full path of your source directory
#	SRCS_OUT:=		the full path of all your source files
#	OBJ_DIR:=		the full path of the object directory
#	TARGET:=		the full path of the generated application
#	LINKS:=			the names of the symlink files
#	OBJS_OUT:=		the output names of all object files
#   SYSTEM_LIBS_OUT:= the full path to the system libs
#
#	Note:
#
#	Todo:
#	* attributes handling
#	* we need a light version of that template that will be provided with TARGET
#       and will take care of the global generic rules only (install crushed ...
#       ftp_install...)
#	* CFLAGS ... ( $(PIC) $(OPTIMIZER)  )
#	* better handling of when x86 apps export symbols

############################################################
# no user-servicable parts beyond this line

# no recursive inclusion
ifdef INCLUDED_HOSTAPPTEMPLATE_INT
$(error a template cannot include itself (HostAppTemplate))
endif
INCLUDED_HOSTAPPTEMPLATE_INT=1

TARGET_NAME_IN:=	$(strip $(TARGET_NAME))
SRC_DIR_IN:=		$(strip $(SRC_DIR))
SRCS_IN:=			$(strip $(SRCS))
MY_CFLAGS_IN:=		$(strip $(MY_CFLAGS))
MY_INCLUDES_IN:=	$(strip $(MY_INCLUDES))
MY_OPTIMIZER_IN :=	$(strip $(MY_OPTIMIZER))
SYSTEM_LIBS_IN:=	$(strip $(SYSTEM_LIBS))
RESOURCE_FORK_IN:=	 $(strip $(RESOURCE_FORK))
RESOURCES_IN:=	 $(strip $(RESOURCES))
MORE_OBJS_IN := $(strip $(MORE_OBJS))
NEEDS_DFLAGS_IN :=	$(strip $(NEEDS_DFLAGS))

# NEEDS_DFLAGS isn't available yet.
NEEDS_DFLAGS_IN :=

# Save variables that are gonna be touched by this makefile
TEMPLATE_ASFLAGS_SAVE_HOSTAPPTEMPLATE_INT:=$(TEMPLATE_ASFLAGS)
# Don't touch inputs to other templates
TEMPLATE_CFLAGS_SAVE_HOSTAPPTEMPLATE_INT:=$(TEMPLATE_CFLAGS)
MY_INSTALL_DIR_SAVE_HOSTAPPTEMPLATE_INT:=$(MY_INSTALL_DIR)

MY_INSTALL_DIR:=$(HOST_TOOLS_BIN)

NEEDS_DFLAGS_INT := 1
ifneq ($(NEEDS_DFLAGS_IN),)
	ifneq ($(subst true,1,$(NEEDS_DFLAGS_IN)),1)
		ifeq ($(subst false,0,$(NEEDS_DFLAGS_IN)),0)
			NEEDS_DFLAGS_INT := 0
		else
			$(warning $(TARGET_NAME) - HostAppTemplate.mk called with \
NEEDS_DFLAGS_IN set to '$(NEEDS_DFLAGS_IN)' which is unknown. \
-- Assuming NEEDS_GLUE_IN:=0)
		endif
	endif
endif

OBJ_DIR:=$(HOST_OBJ_DIR_FROM_SRC_DIR)

SRC_DIR_OUT:=$(SRC_DIR_OUT_FROM_SRC_DIR)
MORE_OBJS_OUT := $(addprefix $(SRC_DIR_OUT)/,$(filter-out /%,$(MORE_OBJS_IN))) $(filter/%,$(MORE_OBJS_IN))

SRCS_OUT:= $(addprefix $(SRC_DIR_OUT)/,$(SRCS))

SYSTEM_LIBS_OUT:= $(call HOST_HANDLE_LIBRARIES, $(SYSTEM_LIBS_IN))
RESOURCES_OUT := $(call FULL_NAMES, $(RESOURCE_FORK_IN) $(RESOURCES_IN))
RESOURCE_FORK_OUT = $(error $(SRC_DIR) - $(TARGET_NAME) -- You should be using RESOURCES_OUT instead of RESOURCE_FORK_OUT) 

TARGET:= $(addprefix $(OBJ_DIR)/,$(TARGET_NAME))

build: $(MY_INSTALL_DIR)/$(TARGET_NAME)

hostbuild: $(MY_INSTALL_DIR)/$(TARGET_NAME)

TEMPLATE_CFLAGS:= ${HOST_APP_CFLAGS}

ifeq ($(NEEDS_DFLAGS_INT),0)
$(TARGET): DFLAGS_SPEC:=
endif

TEMPLATE_ASFLAGS := ${HOST_APP_ASFLAGS}

# -I$(subst :, -I,$(subst ;, -I,$(HOST_BEINCLUDES)))

$(TARGET): USE_HOST_TOOLS:=1

# TARGET := $(TARGET)
# SRC_DIR := $(SRC_DIR)
# SRCS := $(SRCS)
# OBJ_DIR := $(OBJ_DIR)
# MY_CFLAGS := $(MY_CFLAGS)
# MY_INCLUDES := $(MY_INCLUDES)
# MY_OPTIMIZER := $(MY_OPTIMIZER)
include $(SDIR_TEMPLATES)/CompileTemplate.mk
TARGET := $(TARGET_IN)

# FIXME - PPC can be handled fairly easily, however x86 can not
#	The current method of dealling with this when a PPC '.exp' file
#	is present is not a good solution. -- Do we always want those
#	arguments on x86, or do we use another template? (steven)
#
#	I would use micro-managing and deferred variable expansion to
#	implement that. Using platform/cpu/compiler dependent logic
#	somewhere else than in Init.mk should be avoided at all cost.
#	Let''s not add temporary cruft. (marcdlg)
	

#ifneq ($(wildcard $(SRC_DIR_OUT)/$(TARGET_NAME).exp),)
#ifeq ($(CPU), ppc)
#$(TARGET):LDFLAGS:= $(LDFLAGS) -f $(SRC_DIR_OUT)/$(TARGET_NAME).exp
#endif
#
#ifeq ($(CPU), i586)
$(TARGET):LDFLAGS:= $(HOST_LDFLAGS) -Xlinker -h -Xlinker _APP_
#endif
#endif

# Note: If you even /think/ about "simplifying" this by having the following
#  prerequisite in the same place as the link rule, it will break.
#
#  Apparently prerequisites added with the rule are placed at the *beginning*
#  of the list. In cases like this were the glue must be first, this breaks
#  things. -- This seems to only really be problematic when a static library
#  is in SYSTEM_LIBS -- which is fairly common for drivers. (steven 00-07-19)
$(TARGET): $(MORE_OBJS_OUT) $(SYSTEM_LIBS_OUT) $(RESOURCES_OUT)

.DELETE_ON_ERROR: $(TARGET)
$(TARGET):
	$(call PrepareDir, Linking) 
	$(HOST_LD) $(call LINKABLE_ITEMS, $^) $(LDFLAGS) -o "$@" $(WRITE_REPORT)
	$(SETVERSION) $@ $(SYSTEM_VERSION) $(APP_VERSION)
ifneq ($(RESOURCES_OUT),)
	$(call Prepare, Adding resources)
	-$(if $(filter %.rsrc,$^), \
		$(XRES) -o $@ $(filter %.rsrc,$^),)
	-$(if $(filter %.r,$^), \
		$(MWBRES) -merge -o $@ $(filter %.r,$^),)
endif
	$(MIMESET) -f "$@"

# This removes a circular dependancy warning that we take care of in DO_INSTALL_RULE
ifeq ($(filter $(MY_INSTALL_DIR)/$(TARGET_NAME), $(INSTALL_FILE_DEPENDANCIES)),)
$(MY_INSTALL_DIR)/$(TARGET_NAME): $(INSTALL_FILE_DEPENDANCIES)
endif

$(MY_INSTALL_DIR)/$(TARGET_NAME): $(TARGET)
	$(DO_INSTALL_FILE)


## This is a Host-based app, it doesn't get installed
# ifdef IGNORE_TARGET_SPEC
# $(INSTALL_RULES): $(MY_INSTALL_DIR)/$(TARGET_NAME)
# endif
# $(INSTALL_RULES): $(filter $(MY_INSTALL_DIR)/$(TARGET_NAME),$(FILES_SPEC))

# Restore variables
TEMPLATE_ASFLAGS:=$(TEMPLATE_ASFLAGS_SAVE_HOSTAPPTEMPLATE_INT)
# Don't touch inputs to other templates
MY_INSTALL_DIR:=$(MY_INSTALL_DIR_SAVE_HOSTAPPTEMPLATE_INT)
RESOURCE_FORK:=$(RESOURCE_FORK_SAVE_HOSTAPPTEMPLATE_INT)
TEMPLATE_CFLAGS:=$(TEMPLATE_CFLAGS_SAVE_HOSTAPPTEMPLATE_INT)

# no recursive inclusion
INCLUDED_HOSTAPPTEMPLATE_INT=

TARGET_NAME:=	
SRC_DIR:=		
SRCS:=			
MY_CFLAGS:=		
MY_INCLUDES:=	
MY_OPTIMIZER :=	
SYSTEM_LIBS:=	
RESOURCE_FORK:=	
NEEDS_DFLAGS := 
MORE_OBJS :=
