# Makefile using AddonTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the addon. (can not be empty)
TARGET_NAME:=nasmPlugin

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/beide/optional/nasmPlugin

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	Plugin.cpp \
	NASMBuilder.cpp \
	CommandLineOptionsTextView.cpp \
	gccPlugin/source/ELFReader.cpp \
	gccPlugin/source/CommandLineText.cpp \
	gccPlugin/source/PlugInUtil.cpp \
	gccPlugin/source/MTextView.cpp

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/Plugin_API/MWPreferences/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/gccPlugin/source/ \
	-I$(BUILDHOME)/src/inc/os_p/
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your addon links against.
SYSTEM_LIBS:= libroot.so libbe.so

# PARENT_IMAGE - the application image which will be loading this add-on
#                this should set to a variable in SharedBinaries.mk specified
#                in the build tree and *explicitly* _not_ in the install tree.
# PARENT_IMAGE :=

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BEIDE_OPTIONAL)/nasmPlugin

# RESOURCES - the files containing the resources you want to add to your addon
#             Its suffix has to be '.rsrc'.
# RESOURCES := 

include $(SDIR_TEMPLATES)/AddonTemplate.mk

# Makefile using DataTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

#  FILES - the name of the files to copy. (can not be empty)
FILES := ReadMe

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=$(SRC_DIR_IN)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BEIDE_OPTIONAL)/nasmPlugin

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_SYSTEM_DATA)

include $(SDIR_TEMPLATES)/DataTemplate.mk

