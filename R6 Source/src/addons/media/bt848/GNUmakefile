# Makefile using AddonTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the addon. (can not be empty)
TARGET_NAME:=bt848.media_addon

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media/bt848

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	bt848addon.cpp \
	Bt848Controllable.cpp
#	mcontrolbt848.cpp 

MY_DFLAGS := -DNDEBUG=1

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
	-Iheaders \
	-IBt848Source \
	-Ilibvideo \
	-I$(BUILDHOME)/src/inc/canvas_p \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(BUILDHOME)/src/inc/driver \
	-I$(BUILDHOME)/src/inc/tracker_p \
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your addon links against.
SYSTEM_LIBS:= libroot.so libbe.so libtracker.so \
	libmedia.so libBt848Source.a libvideo.a
#

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_ADD_ONS)/media

# LINK_DIRS	 - the locations to link the items (these are symlinks relative to
#			   the INSTALL_BASE. (they'll be relative to /boot after install)
# Generally (if used): $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)
# LINK_DIRS :=

# SIGNATURE - the addon's signature
# SIGNATURE :=

# RESOURCES - the files containing the resources you want to add to your addon
#             Its suffix has to be '.rsrc'.
# RESOURCES :=

# ATTRIBUTES - the command line invoked upon the target through nuaddattr
# ATTRIBUTES :=

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

include $(SDIR_TEMPLATES)/AddonTemplate.mk


include $(call INCLUDE_IF_AVAILABLE, \
	libvideo Bt848Source \
, src/addons/media/bt848)

