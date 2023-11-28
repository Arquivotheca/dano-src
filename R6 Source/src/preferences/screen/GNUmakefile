# Makefile using AppTemplate.mk
# Copyright (c) 2000; Be, Inc. All Rights Reserved.
#
# REMEMBER: You can't comment out any of these, though some of them can be
#  set to empty. (The flat makefile system means if you don't set things to
#  empty, they likely won't be empty when you're called.)

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=Screen

# SRC_DIR - the relative path from $(BUILDHOME) of the root of your
#           application source tree. (can not be empty)
SRC_DIR:= src/preferences/screen

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	screen_utils.cpp \
	screen.cpp


# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := -DUSE_OFFS=0

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES	:= \
				-I$(BUILDHOME)/src/inc/driver \
				-I$(BUILDHOME)/src/inc/app_p \
				-I$(BUILDHOME)/src/inc/app_server_p \
				-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc/support_p \
				-I$(BUILDHOME)/src/inc/interface_p

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libbe.so libroot.so

# RESOURCE_FORK - the file contains the resources you want to add to your app.
#                 Its suffix has to be '.rsrc'. It has to be empty
RESOURCE_FORK	:= $(TARGET_NAME).rsrc

# LINK_DIRS	 - the locations to link the items (these are symlinks relative to
#			   the INSTALL_BASE. (they'll be relative to /boot after install)
# Generally (if used): $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)
LINK_DIRS := $(INSTALL_USER_PREFERENCES) $(INSTALL_USER_PREFMENU)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_PREFERENCES)

#MY_OPTIMIZER:=-O1
include $(SDIR_TEMPLATES)/AppTemplate.mk


