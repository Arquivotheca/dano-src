# Makefile using DataTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

#  FILES - the name of the files to copy. (can not be empty)
FILES := Plugin_API \
	Editor_add_ons_API \
#

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/beide/Plugin_API

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BEIDE_PLUGIN_API)

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_SYSTEM_DATA)

include $(SDIR_TEMPLATES)/DataTemplate.mk

# Makefile using DataTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/beide/Plugin_API/CommentAddOn

#  FILES - the name of the files to copy. (can not be empty)
FILES := $(call ALL_DOT_WHAT, proj cpp)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BEIDE_PLUGIN_API)/$(notdir $(SRC_DIR))

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_USER_DATA)

include $(SDIR_TEMPLATES)/DataTemplate.mk


# Makefile using DataTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/beide/Plugin_API/ShellPlugin

SRC_DIR_OUT := $(call FULL_SRC_DIR, $(SRC_DIR))

#  FILES - the name of the files to copy. (can not be empty)
FILES := $(call ALL_DOT_WHAT, proj cpp h rsrc sh)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BEIDE_PLUGIN_API)/$(notdir $(SRC_DIR))

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_USER_DATA)

include $(SDIR_TEMPLATES)/DataTemplate.mk

# Makefile using DataDirTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

#  SOURCE - the name of the files to copy. (can not be empty)
SOURCE := MWEditorAddOns

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/beide/Plugin_API

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BEIDE_PLUGIN_API)/$(SOURCE)

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_SYSTEM_DATA)

include $(SDIR_TEMPLATES)/DataDirTemplate.mk


# Makefile using DataDirTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

#  SOURCE - the name of the files to copy. (can not be empty)
SOURCE := MWPreferences

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/beide/Plugin_API

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_BEIDE_PLUGIN_API)/$(SOURCE)

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_SYSTEM_DATA)

include $(SDIR_TEMPLATES)/DataDirTemplate.mk

-include $(call INCLUDE_IF_AVAILABLE, \
	ShellPlugin \
	CommentAddOn \
	FilterAddOn \
, src/apps/beide/Plugin_API)
