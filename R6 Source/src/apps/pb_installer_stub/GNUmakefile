# Makefile using AppTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=InstallerStub

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/pb_installer_stub

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	ArcCatalog.cpp \
	CTreeItem.cpp \
	CreatePath.cpp \
	DarkenBitmap.cpp \
	DestManager.cpp \
	DestPreview.cpp \
	DestinatationList.cpp \
	DoError.cpp \
	FSIcons.cpp \
	FileTree.cpp \
	FindChoiceWind.cpp \
	HelpButton.cpp \
	HelpWindow.cpp \
	IArchiveItem.cpp \
	IArchivePatchItem.cpp \
	IArchiveScriptItem.cpp \
	IGroupList.cpp \
	IGroupsListView.cpp \
	IMenuBar.cpp \
	IStatusWindow.cpp \
	IconMenuItem.cpp \
	InstallApp.cpp \
	InstallBuildGroupList.cpp \
	InstallLooper.cpp \
	InstallPack--Attrib.cpp \
	InstallPack.cpp \
	InstallUtil.cpp \
	InstallView.cpp \
	InstallWindow.cpp \
	LabelView.cpp \
	LicenseView.cpp \
	LicenseWindow.cpp \
	Log.cpp \
	LogEntry.cpp \
	LogListView.cpp \
	LogView.cpp \
	LogWindow.cpp \
	MMallocIO.cpp \
	PCodec.cpp \
	PackAttrib.cpp \
	PackData.cpp \
	PackageDB.cpp \
	PackageItem.cpp \
	PatchInfoWind.cpp \
	PositionWindow.cpp \
	PreviewWind.cpp \
	REntryList.cpp \
	ReplaceDialog.cpp \
	SAttrIO.cpp \
	SPackData.cpp \
	SerialNum.cpp \
	SettingsManager.cpp \
	SimpleListView.cpp \
	SplashWindow.cpp \
	TreeItem.cpp \
	TreeView.cpp \
	UpgradeList.cpp \
	ZlCodec.cpp \
	DummyAttrIO.cpp \
	DummySResIO.cpp \
#

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I. \
	-I$(call FULL_SRC_DIR, src/apps/software_valet) \
	-I${BUILDHOME}/srcx/gnu/zlib-1.1.3 \
	-I$(call FULL_SRC_DIR, src/apps/package_builder) \
	-I${BUILDHOME}/src/kit/3d \
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so libz.a \
	libtracker.so libtranslation.so \
	libsocket.so libbind.so

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

