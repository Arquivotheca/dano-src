# Makefile using AddonTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the addon. (can not be empty)
TARGET_NAME:=MWPrefsPlugin

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/beide/MWPlugin

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	source/MPlugIn.cpp \
	source/MmwccBuilder.cpp \
	source/MLinkerBuilder.cpp \
	source/MDisassembleView.cpp \
	source/MProcessorView.cpp \
	source/MPEFView.cpp \
	source/MLinkerView.cpp \
	source/MProjectPrefsView.cpp \
	source/MLanguageView.cpp \
	source/MWarningsView.cpp \
	source/MGlobalOptsView.cpp \
	IDE.f/Preferences/MDefaultPrefs.cpp \
	IDE.f/Preferences/MPrefsStruct.cpp \
	IDE.f/Utilities/CString.cpp \
	IDE.f/Interface/MTextView.cpp \
	IDE.f/Interface/MTextControl.cpp \
	IDE.f/Interface/MTextControlFilter.cpp \
	source/MTargeter.cpp

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Interface/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Build/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Preferences/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/Plugin_API/MWPreferences/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Utilities/Headers/ \
	-I$(BUILDHOME)/src/apps/beide/IDE.f/Find/Headers/ \
	-I$(call FULL_SRC_DIR, $(SRC_DIR))/source \
#

# MY_OPTIMIZER - the prefered optimization level


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
ifeq ($(TARGET_CPU), ppc)
	MY_INSTALL_DIR := $(INSTALL_BEIDE_PLUGINS)/Prefs_add_ons
else
	MY_INSTALL_DIR := $(INSTALL_BEIDE_CROSSDEV)
endif

# RESOURCES - the files containing the resources you want to add to your addon
#             Its suffix has to be '.rsrc'.
RESOURCES := mwplugin.rsrc

include $(SDIR_TEMPLATES)/AddonTemplate.mk

-include $(call INCLUDE_IF_AVAILABLE, \
	mwbres \
, $(SRC_DIR_IN))
