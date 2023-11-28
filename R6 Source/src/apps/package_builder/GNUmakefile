# Makefile using AppTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=PackageBuilder

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/package_builder

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS :=  \
	ArchiveFileItem.cpp \
	ArchiveFolderItem.cpp \
	ArchiveLinkItem.cpp \
	ArchivePatchItem.cpp \
	ArchiveScriptItem.cpp \
	Attributes.cpp \
	ChildWindow.cpp \
	ColLabelView.cpp \
	CopyFile.cpp \
	CreatePath.cpp \
	DatatypesAddOn.cpp \
	DestListView.cpp \
	DestView.cpp \
	DestinationList.cpp \
	DestinationWindow.cpp \
	DoError.cpp \
	ExtractUtils.cpp \
	FArchiveItem.cpp \
	FListView.cpp \
	FileLooper.cpp \
	FilePatcher.cpp \
	FilePatcherEng.cpp \
	FolderCalc.cpp \
	GStatusWind.cpp \
	GlobalIcons.cpp \
	GroupList.cpp \
	GroupsListView.cpp \
	GroupsView.cpp \
	GroupsWindow.cpp \
	HeapWindow.cpp \
	IconCache.cpp \
	IconMenuItem.cpp \
	InstSetWindow.cpp \
	LabelView.cpp \
	MFilePanel.cpp \
	MMallocIO.cpp \
	MThread.cpp \
	MemPrint.cpp \
	NameDialog.cpp \
	PCodec.cpp \
	PackApplication.cpp \
	PackArc.cpp \
	PackData.cpp \
	PackFile--Installer.cpp \
	PackFileAttrib.cpp \
	PackList--AddPatch.cpp \
	PackListView.cpp \
	PackWindow.cpp \
	PackageSetWind.cpp \
	PatchThread.cpp \
	PatchWindow.cpp \
	PositionWindow.cpp \
	PrefsWindow.cpp \
	Replace.cpp \
	RWindow.cpp \
	SAttrIO.cpp \
	SPackData.cpp \
	STextField.cpp \
	SerialNum.cpp \
	SettingsView.cpp \
	SmallPopup.cpp \
	SplashView.cpp \
	StatusWindow.cpp \
	TCheckBox.cpp \
	ZlCodec.cpp \

MY_DFLAGS := -DPACKAGEBUILDER

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
	-I$(BUILDHOME)/src/kit/translation \
	-I$(BUILDHOME)/src/inc/media_p \
	-I${BUILDHOME}/srcx/gnu/zlib-1.1.3 \
#

# MY_OPTIMIZER - the prefered optimization level


# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so libz.a \
	libsocket.so libbind.so libtracker.so \
	libtranslation.so

# RESOURCE_FORK - the file contains the resources you want to add to your app.
#                 Its suffix has to be '.rsrc'. It has to be empty
RESOURCE_FORK := $(TARGET_NAME).rsrc

# LINK_DIRS	 - the locations to link the items (these are symlinks relative to
#			   the INSTALL_BASE. (they'll be relative to /boot after install)
# Generally (if used): $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)
LINK_DIRS := $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_DEVELOP)/PackageBuilder

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

# This will over-ride the application's version id.
# MY_VERSION := 

include $(SDIR_TEMPLATES)/AppTemplate.mk

