# Makefile using AddonTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the addon. (can not be empty)
TARGET_NAME:=Commenter

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/beide/Plugin_API/CommentAddOn

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := $(patsubst $(SRC_DIR)/%,%, \
$(SRC_DIR)/Comment.cpp \
)

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := \
	-I$(BUILDHOME)/src/apps/beide/Plugin_API/MWEditorAddOns/
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
MY_INSTALL_DIR := $(INSTALL_BEIDE_PLUGINS)/Editor_add_ons

# RESOURCES - the files containing the resources you want to add to your addon
#             Its suffix has to be '.rsrc'.
# RESOURCES := 

include $(SDIR_TEMPLATES)/AddonTemplate.mk

