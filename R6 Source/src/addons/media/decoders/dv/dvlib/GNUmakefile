# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libbedv.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media/decoders/dv/dvlib

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := d1rgb.asm e1rgb.asm \
	common/src/d1rgbq.c common/src/e1rgbq.c dvcodec.cpp


MY_DFLAGS := 

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I. \
	-Icommon/inc \
#

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. 
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.a
# LINKS :=

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

MY_AS_FLAGS := 

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

# Makefile using SimpleSharedLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=csedv-c.so

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=$(SRC_DIR_IN)

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS:= csedv/src/dct.c csedv/src/decdv.c \
	csedv/src/encdv.c csedv/src/table.c

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I. \
	-Icommon/inc \
	-Icsdev/src
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so

# WHOLE_ARCHIVES - This lists static libraries that you want to completely
#                  include in your shared library. Absolute paths are available,
#                  otherwise a default path of $(BELIBRARIES) is used.
# WHOLE_ARCHIVES :=

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. 
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
MY_INSTALL_DIR := $(INSTALL_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.so
# LINKS :=

# PROFILE_ME - If you should profile this shared lib, set this to
#              'true' or '1'. -- False is assumed if empty, '0' or 'false'.
# PROFILE_ME := 1

# UNDEFINED_SYMBOLS - list of symbols which start as undefined.
#                     Through the linker's -u or --undefined (in GCC) options.
# UNDEFINED_SYMBOLS := 

# MORE_OBJS - additional objects to link to the target
#             these may be absolute paths and are not cleaned up
# MORE_OBJS :=

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

# RESOURCES - the file(s) containing the resources you want to add to your target.
#             The suffix(es) should be '.rsrc' or '.r'.
# RESOURCES :=

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)/lib
SOURCE:=csedv.so
TARGET:= $(BELIBRARIES)/$(SOURCE)
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/HostDataFileTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:=$(SOURCE_IN)
TARGET:= $(INSTALL_LIBS)/$(SOURCE)
ATTRIBUTES:=
PERMISSIONS:=$(PERMS_SYSTEM_DATA)
ELF_FILES+=$(TARGET)
MY_DEPS:=$(call FULL_SRC_DIR, $(src/addons/media/decoders/dv/dvlib))/$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

ifneq ($(call MFLAG_IS_SET, DEVELOP_LIBS_ARE_FILES),)


SRC_DIR:=$(SRC_DIR_IN)
SOURCE:=$(SOURCE_IN)
TARGET        := $(INSTALL_DEV_LIBS)/$(SOURCE_IN)
ATTRIBUTES    := 
PERMISSIONS   := $(PERMS_SYSTEM_DATA)
ELF_FILES+=$(TARGET)
MY_DEPS:=$(call FULL_SRC_DIR, $(src/addons/media/decoders/dv/dvlib))/$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk	

else

SRC_DIR := $(SRC_DIR_IN)
SOURCE:=$(INSTALL_LIBS)/csedv.so
LINK_PREFIX:=
LINKS := $(INSTALL_DEV_LIBS)/csedv.so
LINK_RELATION:=$(INSTALL_BASE)
MY_DEPS:=$(call FULL_SRC_DIR, $(src/addons/media/decoders/dv/dvlib))/$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk	

endif

