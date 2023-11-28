SRC_DIR:=src/addons/media/decoders/indeo5
MY_OBJ_OFFSET := indeo5.encoder.f
SRCS := \
		beoshive.c
#
TYPE := SHARED


MY_DFLAGS := -D__NO_WINDOWS__

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, src/addons/media/encoders/indeo5) \
	-I$(BUILDHOME)/src/addons/media/decoders/indeo5/include \
	-I$(BUILDHOME)/src/inc/media_p
#

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

MY_DFLAGS := $(MY_DFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)

# Makefile using AddonTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the addon. (can not be empty)
TARGET_NAME:=indeo5.encoder

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/media/encoders/indeo5

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := Indeo5Encoder.cpp  \
        HiveEncoder.cpp \
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your addon links against.
SYSTEM_LIBS:= libroot.so libbe.so \
	libmedia.so libindeo_enc.a
#

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_ADD_ONS)/media/encoders

include $(SDIR_TEMPLATES)/AddonTemplate.mk

include $(call INCLUDE_IF_AVAILABLE, encode.cpk, $(SRC_DIR_IN))
