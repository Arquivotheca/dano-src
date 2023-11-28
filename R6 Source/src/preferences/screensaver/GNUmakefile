# Makefile using AppTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=ScreenSaver

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/preferences/screensaver

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	BlanketApp.cpp \
	CMonitorControl.cpp \
	DeepCopy.cpp \
	FadeView.cpp \
	MockupView.cpp \
	ModuleListItem.cpp \
	ModuleListView.cpp \
	ModuleRoster.cpp \
	ModuleView.cpp \
	ModulePreviewView.cpp \
	Settings.cpp \
	SetupWindow.cpp \
	Sliders.cpp \
	PasswordPanel.cpp \
	main.cpp \
#

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
	-I$(call FULL_SRC_DIR, $(SRC_DIR)/screenSaverCommon) \
	-I$(BUILDHOME)/headers/add-ons/screen_saver \
#

# MY_OPTIMIZER - the prefered optimization level


# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libtracker.so libbe.so screensaverlib.a

# RESOURCE_FORK - the file contains the resources you want to add to your app.
#                 Its suffix has to be '.rsrc'. It has to be empty
RESOURCE_FORK := $(TARGET_NAME).rsrc

# LINK_DIRS	 - the locations to link the items (these are symlinks relative to
#			   the INSTALL_BASE. (they'll be relative to /boot after install)
# Generally (if used): $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)
LINK_DIRS := $(INSTALL_USER_PREFMENU) $(INSTALL_USER_PREFERENCES)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_PREFERENCES)

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

# This will over-ride the application's version id.
# MY_VERSION := 

include $(SDIR_TEMPLATES)/AppTemplate.mk


include $(BUILDHOME)/src/preferences/screensaver/screenSaverCommon/$(MAKEFILE_LEAFNAME)

