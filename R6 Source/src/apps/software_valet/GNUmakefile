# Makefile using AppTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=SoftwareValet

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/software_valet

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS :=  \
	ArcCatalog.cpp \
	AttrIO.cpp \
	BTSAddress.cpp \
	BTSBuffSocket.cpp \
	BTSSocket.cpp \
	BackupPanel.cpp \
	BufPlay.cpp \
	CTreeItem.cpp \
	ColLabelView.cpp \
	CommSetView.cpp \
	CopyFile.cpp \
	CreatePath.cpp \
	DarkenBitmap.cpp \
	DestManager.cpp \
	DestPreview.cpp \
	DestinatationList.cpp \
	DoError.cpp \
	DoIcons.cpp \
	DownloadWindow.cpp \
	FSIcons.cpp \
	FileTree.cpp \
	FindChoiceWind.cpp \
	GetUpdate.cpp \
	GraphicButton.cpp \
	HTConnection.cpp \
	HTEscape.cpp \
	HTHeaders.cpp \
	HeapWindow.cpp \
	HelpButton.cpp \
	HelpFilter.cpp \
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
	InstallListView.cpp \
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
	LogPrefsView.cpp \
	LogView.cpp \
	LogWindow.cpp \
	MFilePanel.cpp \
	MMallocIO.cpp \
	MThread.cpp \
	ManagerListView.cpp \
	ManagerWindow.cpp \
	MultiScrollBar.cpp \
	NameDialog.cpp \
	PCodec.cpp \
	PackAttrib.cpp \
	PackData.cpp \
	PackFilePanel.cpp \
	PackageDB.cpp \
	PackageItem.cpp \
	PackageWindow.cpp \
	PatchInfoWind.cpp \
	PositionWindow.cpp \
	PreviewWind.cpp \
	ProgressBar.cpp \
	REntryList.cpp \
	RWindow.cpp \
	RegInfoView.cpp \
	RegisterThread.cpp \
	RegisterView.cpp \
	RegisterWindow.cpp \
	ReplaceDialog.cpp \
	SAttrIO.cpp \
	SPackData.cpp \
	SResIO.cpp \
	SUrlView.cpp \
	SerialNum.cpp \
	ServicesDialog.cpp \
	SettingsManager.cpp \
	SettingsWindow.cpp \
	SetupWindow.cpp \
	SimpleListView.cpp \
	SplashWindow.cpp \
	StatusDialog.cpp \
	TCheckBox.cpp \
	TTabView.cpp \
	TreeItem.cpp \
	TreeView.cpp \
	TriangleTab.cpp \
	Troll.cpp \
	UninstallThread.cpp \
	UninstallTree.cpp \
	UninstallWindow.cpp \
	UpdateDialog.cpp \
	UpdateThread.cpp \
	UpdateWindow.cpp \
	UpgradeList.cpp \
	VHTUtils.cpp \
	ValetApp.cpp \
	ValetWindow.cpp \
	ValetVersion.cpp \
	ZlCodec.cpp \
	colors.cpp \
	memglue.c \
	readline.cpp \
#

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := \
	-I$(BUILDHOME)/src/translation \
	-I. \
	-I$(BUILDHOME)/src/inc/media_p \
	-I${BUILDHOME}/srcx/gnu/zlib-1.1.3 \
	-Ipackage_builder
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so libtracker.so \
	libz.a libsocket.so libbind.so libtranslation.so

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
MY_INSTALL_DIR := $(INSTALL_APPS)

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

# This will over-ride the application's version id.
# MY_VERSION := 

include $(SDIR_TEMPLATES)/AppTemplate.mk

