
TARGET_NAME := bdb.rsrc

SRC_DIR:=src/apps/bdb

SRCS := \
	RButtons.r \
	RMemoryWindow.r \
	RRegInfo.r \
	RRuntimeSupport.r \
	RSCTables.r \
	RTeamWindow.r \
	RThreadWindow.r \
	bdb.mimeinfo bdb.icon

include $(SDIR_TEMPLATES)/RezTemplate.mk

#
# bdb makefile
#
# Copyright (C) 2001 Be Inc.  All Rights Reserved
#

RESOURCES := $(TARGET)

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=bdb

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/bdb

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS :=  \
	DAboutWindow.cpp \
	DBreakpoint.cpp \
	DBreakpointWindow.cpp \
	DCpuState.cpp \
	DDebugApp.cpp \
	DDebugMessageWin.cpp \
	DDwarf2.cpp \
	DDwarfSymFile.cpp \
	DDwarfType.cpp \
	DElf.cpp \
	DElfSymFile.cpp \
	DExprParser.cpp \
	DFindDialog.cpp \
	DFunction.cpp \
	DGetMemoryLocation.cpp \
	DInternalProxy.cpp \
	DListBox.cpp \
	DLocalNub.cpp \
	DLocalx86Nub.cpp \
	DLocation.cpp \
	DMainWindow.cpp \
	DMemoryWindow.cpp \
	DNub.cpp \
	DPulseView.cpp \
	DRegsWindow.cpp \
	DRemoteProxyNub.cpp \
	DResizer.cpp \
	DRuntimeSupport.cpp \
	DSetWatchpoint.cpp \
	DSettingsDialog.cpp \
	DShowAddress.cpp \
	DShowFunction.cpp \
	DSourceCode.cpp \
	DSourceCode.scan.cpp \
	DSourceFileTable.cpp \
	DSourceView.cpp \
	DStackCrawl.cpp \
	DStackCrawlView.cpp \
	DStackCrawlWindow.cpp \
	DStackFrame.cpp \
	DStatement.cpp \
	DStmtMachine.cpp \
	DSymFile.cpp \
	DSymWorld.cpp \
	DTeam.cpp \
	DTeamWindow.cpp \
	DThread.cpp \
	DThreadWindow.cpp \
	DTracepoint.cpp \
	DType.cpp \
	DUtils.cpp \
	DVariable.cpp \
	DVariableItem.cpp \
	DViewAsDialog.cpp \
	DWaitWindow.cpp \
	DWatchpoint.cpp \
	DWatchpointWindow.cpp \
	DWindow.cpp \
	Dx86Nub.cpp \
	Dx86StackFrame.cpp \
	XView.cpp

SRCS += \
	HAppResFile.cpp \
	HButtonBar.cpp \
	HColorControl.cpp \
	HColorDemo.cpp \
	HColorPicker2.cpp \
	HColorSlider.cpp \
	HColorSquare.cpp \
	HColorUtils.cpp \
	HDialog.cpp \
	HDlogView.cpp \
	HError.cpp \
	HHelpWindow.cpp \
	HPreferences.cpp \
	HStream.cpp \
	HTabSheet.cpp

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := -Wno-overloaded-virtual

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES += -I$(BUILDHOME)/src/inc/bdb_p \
	-IAddOnSources
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so libsocket.so libbind.so \
	libmedia.so libtracker.so libdevel.a libtcpmessage.so libudpmessage.so $(LIBSTDCPP_OBJECT)

# LINK_DIRS	 - the locations to link the items (these are symlinks relative to
#			   the INSTALL_BASE. (they'll be relative to /boot after install)
# Generally (if used): $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)
LINK_DIRS := $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_DEVELOP)/bdb

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

MY_DEBUG_FLAGS := -g -DDEBUG=1

# This will over-ride the application's version id.
# LATEST_CHANGE:=$(shell p4 changes -m 1 //depot/main/src/apps/bdb/... | awk '{print $$2}')
MAJOR_VERSION:=2
MINOR_VERSION:=0
MY_VERSION 	:= -app $(MAJOR_VERSION) $(MINOR_VERSION) 0 f0 -short $(MAJOR_VERSION).$(MINOR_VERSION).0 -long "$(MAJOR_VERSION).$(MINOR_VERSION) "`echo -n -e '\302\251'`"1999-2001 Be Incorporated"

MY_DFLAGS += \
	-DMAJOR_VERSION=$(MAJOR_VERSION) \
	-DMINOR_VERSION=$(MINOR_VERSION)

include $(SDIR_TEMPLATES)/AppTemplate.mk

-include $(call INCLUDE_IF_AVAILABLE, $(SRC_DIR_IN)/AddOnSources)

