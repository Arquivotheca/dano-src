# Makefile using AppTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=SoundRecorder

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/apps/soundrecorder

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	appsrc/RecorderApp.cpp \
	appsrc/RecorderWin.cpp \
	appsrc/RenderScope.cpp \
	appsrc/ResampleArray.cpp \
	appsrc/main.cpp \
	views/DrawingTidbits.cpp \
	views/FileListView.cpp \
	views/MultiThumbMediaSlider.cpp \
	views/NuTransportView.cpp \
	views/ScopeView.cpp \
	views/SndInfoView.cpp \
	views/TimeDisplay.cpp \
	views/TransportButton.cpp \
	views/VolumeSlider.cpp \
	views/multithumbslider.cpp \
	views/KnobSwitch.cpp \
	views/RenderView.cpp \
	views/LevelMeter.cpp \
	views/BitmapButton.cpp \
	mplayer/MediaController.cpp \
	mplayer/NodeWrapper.cpp \
	mplayer/VideoConsumerNode.cpp \
	mplayer/VideoView.cpp \
	mplayer/MediaTrackController.cpp \
	sndcapsrc/SoundConsumer.cpp \
	sndcapsrc/SoundUtils.cpp \

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := \
	-I$(BUILDHOME)/src/inc/support_p \
	-I$(call FULL_SRC_DIR, $(SRC_DIR))/appsrc \
	-I$(call FULL_SRC_DIR, $(SRC_DIR))/mplayer \
	-I$(call FULL_SRC_DIR, $(SRC_DIR))/sndcapsrc \
	-I$(call FULL_SRC_DIR, $(SRC_DIR))/views \
#

# MY_OPTIMIZER - the prefered optimization level


# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so \
	libmedia.so libtracker.so

RESOURCES := $(TARGET_NAME).rsrc

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

