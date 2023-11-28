# Makefile using AddonTemplate.mk
# Copyright (c) 2000; Be, Inc. All Rights Reserved.
#
# REMEMBER: You can't comment out any of these, though some of them can be
#  set to empty. (The flat makefile system means if you don't set things to
#  empty, they likely won't be empty when you're called.)

# TARGET_NAME - the name of the addon. (can not be empty)
TARGET_NAME:=neomagic

# SRC_DIR - the relative path from of your sources (can not be empty)
SRC_DIR:= src/addons/app_server/$(TARGET_NAME)

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS		:= $(TARGET_NAME).c

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := -DJOE=1

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(BUILDHOME)/$(SRC_DIR) \
				-I$(BUILDHOME)/src/inc \
				-I$(BUILDHOME)/src/inc/os_p

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your addon links against.
SYSTEM_LIBS:= libroot.so

# PARENT_IMAGE - the application image which will be loading this add-on
#                this should set to a variable in SharedBinaries.mk specified
#                in the build tree and *explicitly* _not_ in the install tree.
PARENT_IMAGE :=

# HAS_MAIN - If the add-on can be run as an application, set this to
#            'true' or '1'. -- False is assumed if empty, '0' or 'false'.
HAS_MAIN :=

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_ADD_ONS)/app_server

# LINK_DIRS	 - the locations to link the items (these are symlinks relative to
#			   the INSTALL_BASE. (they'll be relative to /boot after install)
# Generally (if used): $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)
LINK_DIRS :=

# SIGNATURE - the addon's signature
SIGNATURE :=

# RESOURCES - the files containing the resources you want to add to your addon
#             Its suffix has to be '.rsrc'.
RESOURCES :=

# ATTRIBUTES - the command line invoked upon the target through nuaddattr
ATTRIBUTES :=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
