# Makefile using SimpleSharedLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libmidi.so

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/kit/midi

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS :=  \
		Midi.cpp \
		MidiStore.cpp \
		MidiText.cpp \
		MidiPortGlue.cpp \
		SynthGlue.cpp \
		SynthUtils.cpp

# MY_CFLAGS - Template User provided flags.
MY_DFLAGS := -DDEBUG=1 -DX_PLATFORM=X_BE


# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(BUILDHOME)/headers \
			-I$(BUILDHOME)/headers/midi \
			-I$(BUILDHOME)/headers/midi2 \
			-I$(BUILDHOME)/src/kit/bae \
			-I$(call FULL_SRC_DIR, $(SRC_DIR)) \
#

# MY_OPTIMIZER - the prefered optimization level


# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so libmidi2.so libmedia.so libbae.so

# WHOLE_ARCHIVES - This lists static libraries that you want to completely
#                  include in your shared library. Absolute paths are available,
#                  otherwise a default path of $(BELIBRARIES) is used.
# WHOLE_ARCHIVES :=

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. 
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
MY_INSTALL_DIR := $(INSTALL_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.so
LINKS := $(INSTALL_DEV_LIBS)/$(TARGET_NAME)

# PROFILE_ME - If you should profile this shared lib, set this to
#              'true' or '1'. -- False is assumed if empty, '0' or 'false'.
# PROFILE_ME := 1

# UNDEFINED_SYMBOLS - list of symbols which start as undefined.
#                     Through the linker's -u or --undefined (in GCC) options.
# UNDEFINED_SYMBOLS := 

# MORE_OBJS - additional objects to link to the target
#             these may be absolute paths and are not cleaned up
# MORE_OBJS :=

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

