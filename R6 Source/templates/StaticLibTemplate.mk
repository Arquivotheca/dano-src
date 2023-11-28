# StaticLibTemplate.mk -- Static Library Template Makefile
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.
#
# Input Parameters: (they have to set before the template inclusion) 
#  *TARGET_NAME		:=	the name of the Shared Library.
#   MY_INSTALL_DIR	:= 	the full path of the directory you want
#						your app to be installed in. You should use of the
#						globally define INSTALL_* variables define in Init.mk.
#  *SRC_DIR		:=	the relative path from $BUILDHOME of the root of the kit
#					source tree
#  *SRCS		:=	the list of source files relative to $(SRC_DIR). 
#	MY_CFLAGS:=		Template User provided flags. OPTIONAL
#   MY_INCLUDES:=	Additional headers (as might be appended to CFLAGS)
#	MY_OPTIMIZER :=	the prefered optimizer setting
#   SYSTEM_LIBS	:=	the list of system libraries
#                   that your application links against.
#	LINKS		:=	full path to symlinked name pointing to the installed item,
#					used to support compatibility symlinks as seen on other systems.
#					Generally (if used): $(INSTALL_DEV_LIBS)/libFOO.1.a
#   PROFILE_ME  :=  If you should profile this add-on, set this to
#					'true' or '1'. -- False is assumed if empty, '0' or 'false'.
#	MORE_OBJS	:=	additional objects to link to the target
#					these may be absolute paths and are not cleaned up
#	PRIVATE_SOURCES := a value of "true" means that the sources are
#					private and should not be given away even in a
#					restricted source environment
#	README			XXX jbq document this
#
# All the input parameters must exist. They should be set empty if they're
# unneeded. Items preceded with an asterisk (*) can not legally be empty.
#
# Output Parameters: (they become valid after the template inclusion, and are
# invalidated by any other template inclusion)
#
#	TARGET		:=	the full path of the generated application
#   SYSTEM_LIBS_OUT	:=	the full path to the system libs
#
#	Todo:
#	* attributes handling
#	* we need a light version of that template that will be provided with TARGET
#       and will take care of the global generic rules only (install crushed ...
#       ftp_install...)
#	* better handling of when x86 apps export symbols

# no recursive inclusion
ifdef INCLUDED_STATICLIBTEMPLATE_INT
$(error a template cannot include itself (StaticLibTemplate))
endif
INCLUDED_STATICLIBTEMPLATE_INT=1

TARGET_NAME_IN		:=	$(strip $(TARGET_NAME))
SRC_DIR_IN		:=	$(strip $(SRC_DIR))
SRCS_IN		:=	$(strip $(SRCS))
MY_CFLAGS_IN	:= $(strip $(MY_CFLAGS))
MY_INCLUDES_IN := $(strip $(MY_INCLUDES))
MY_OPTIMIZER_IN :=	$(strip $(MY_OPTIMIZER))
SYSTEM_LIBS_IN	:=	$(strip $(SYSTEM_LIBS))
LINKS_IN		:=	$(strip $(LINKS))
MY_INSTALL_DIR_IN	:= 	$(strip $(MY_INSTALL_DIR))
PROFILE_ME_IN	:=	$(strip $(PROFILE_ME))
MORE_OBJS_IN := $(strip $(MORE_OBJS))
PRIVATE_SOURCES_IN := $(strip $(PRIVATE_SOURCES))
README_IN := $(strip $(README))

# Strip the variables -- We don't need to save before hand because nothing is changing.
SRC_DIR:=$(strip $(SRC_DIR))
LINKS:=$(strip $(LINKS))

TEMPLATE_ASFLAGS_STATICLIBTEMPLATE_INT := $(TEMPLATE_ASFLAGS)
# Don't touch inputs to other templates
OBJ_DIR_STATICLIBTEMPLATE_INT := $(OBJ_DIR)
TEMPLATE_CFLAGS_STATICLIBTEMPLATE_INT := $(TEMPLATE_CFLAGS)
# save variables that'll be cleared by a template inclusion
README_SAVE_STATICLIBTEMPLATE_INT := $(README)

OBJ_DIR:=$(OBJ_DIR_FROM_SRC_DIR)

TEMPLATE_CFLAGS:= $(STATIC_CFLAGS) $(PUBLIC_INCLUDES)
TEMPLATE_ASFLAGS := $(STATIC_ASFLAGS)

SRC_DIR_OUT:=$(SRC_DIR_OUT_FROM_SRC_DIR)
SYSTEM_LIBS_OUT:= $(call HANDLE_LIBRARIES, $(SYSTEM_LIBS_IN))
MORE_OBJS_OUT := $(addprefix $(SRC_DIR_OUT)/,$(filter-out /%, $(MORE_OBJS_IN))) $(filter /%,$(MORE_OBJS_IN))

TARGET:= $(call CREATED_LIBRARY_PATH)/$(TARGET_NAME_IN)

ifeq ($(CHECK_PROFILING),1)
	TEMPLATE_CFLAGS += $(PROFILE_FLAG)
	SYSTEM_LIBS_OUT += $(PROFILE_GLUE)
endif

build: $(TARGET)

# TARGET := $(TARGET)
# SRC_DIR := $(SRC_DIR)
# SRCS := $(SRCS)
# OBJ_DIR := $(OBJ_DIR)
# MY_CFLAGS := $(MY_CFLAGS)
# MY_INCLUDES := $(MY_INCLUDES)
# MY_OPTIMIZER := $(MY_OPTIMIZER)
README :=
include $(SDIR_TEMPLATES)/CompileTemplate.mk
TARGET := $(TARGET_IN)
PRIVATE_SOURCES := $(PRIVATE_SOURCES_IN)
README := $(README_SAVE_STATICLIBTEMPLATE_INT)

ARFLAGS:= -crs
$(TARGET): ARFLAGS:= $(ARFLAGS)
$(TARGET): PRIVATE_SOURCES:= $(PRIVATE_SOURCES)

$(TARGET): $(MORE_OBJS_OUT) $(MORE_GLUE_INT) $(SYSTEM_LIBS_OUT)

$(TARGET): README:=$(addprefix $(SRC_DIR_OUT)/,$(README))

.DELETE_ON_ERROR: $(TARGET)
$(TARGET): 
	$(call PrepareDir, Staticly linking)
	$(RM) "$@"
	$(AR) $(ARFLAGS) "$@" $^ $(WRITE_REPORT)
ifneq ($(call MFLAG_IS_SET, DO_SETVERSION),)
	-$(SETVERSION) $@ $(SYSTEM_VERSION) $(APP_VERSION)
endif
	$(LIST_DEPENDENCIES)
	$(LIST_PRIVATE_OBJECTS)

$(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN): $(INSTALL_FILE_DEPENDANCIES)

# Be-internal static libraries won't be installed (steven)
ifneq ($(MY_INSTALL_DIR_IN),)
$(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN): $(TARGET)
	$(DO_INSTALL_FILE)

ifdef IGNORE_TARGET_SPEC
$(INSTALL_RULES): $(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN)
buildlibs: $(TARGET)
else
$(INSTALL_RULES): $(filter $(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN),$(FILES_SPEC))
FILES_SPEC:= $(filter-out $(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN),$(FILES_SPEC))
endif

SOURCE:=$(MY_INSTALL_DIR_IN)/$(TARGET_NAME_IN)
LINK_PREFIX:=
# LINKS += $(addsuffix /$(TARGET_NAME_IN),$(LINK_DIRS))
ifneq ($(strip $(LINKS)),)
LINK_RELATION:=$(if $(filter-out $(MY_INSTALL_DIR_IN)/%,$(LINKS)),$(INSTALL_BASE),$(MY_INSTALL_DIR_IN))
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk
endif
endif

TEMPLATE_ASFLAGS := $(TEMPLATE_ASFLAGS_STATICLIBTEMPLATE_INT)
# Don't touch inputs to other templates
OBJ_DIR := $(OBJ_DIR_STATICLIBTEMPLATE_INT)
TEMPLATE_CFLAGS := $(TEMPLATE_CFLAGS_STATICLIBTEMPLATE_INT)

# no recursive inclusion
INCLUDED_STATICLIBTEMPLATE_INT=


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
MORE_OBJS :=
PRIVATE_SOURCES :=
README :=
