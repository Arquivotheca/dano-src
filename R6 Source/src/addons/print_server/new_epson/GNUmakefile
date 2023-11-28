#makefile for epson printer driver add-ons

SRC_DIR := src/addons/print_server/new_epson

TARGET_NAME := Epson

# MSIX_COLORS	 : Include 6 colors printer code
# MFOUR_COLORS	 : Include 4 colors printer code
# M_MMX			 : Include MMX code
# M_NO_MMX		 : Include no MMX code
# PRINTING_FOR_DESKTOP

MY_CFLAGS := -DM_MMX -DMFOUR_COLORS

SRCS :=	Driver.cpp \
		DriverEpsonModule.cpp \
		MPrinter.cpp \
		MCS_Conversion.cpp \
		MCS_Conversion4.cpp \
		MCS_Conversion6.cpp \
		DitheringObjectBase.cpp \
		raster.cpp \
		raster360.cpp \
		rasterNSlices.cpp

MY_INCLUDES := -I. -Iadd_ons

SYSTEM_LIBS := libroot.so libbe.so libepson.so
PARENT_IMAGE :=
HAS_MAIN :=
MY_INSTALL_DIR := $(INSTALL_ADD_ONS)/Print
LINK_DIRS :=
SIGNATURE :=
RESOURCES :=
ATTRIBUTES :=

ifneq ($(call MFLAG_IS_SET, PRINTING_FOR_DESKTOP),)
MY_CFLAGS += -DPRINTING_FOR_DESKTOP=1
MY_CFLAGS += -DM_NO_MMX -DMSIX_COLORS
else
MY_CFLAGS += -DPRINTING_FOR_DESKTOP=0
endif

include $(SDIR_TEMPLATES)/AddonTemplate.mk

# Makefile using SimpleSharedLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libepson.so

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=$(SRC_DIR_IN)

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS:=MDefinePrinter.cpp

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. 
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
MY_INSTALL_DIR := $(INSTALL_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.so
LINKS := $(INSTALL_DEV_LIBS)/$(TARGET_NAME)

# RESOURCES - the file(s) containing the resources you want to add to your target.
#             The suffix(es) should be '.rsrc' or '.r'.
# RESOURCES :=

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

include $(call INCLUDE_IF_AVAILABLE, add_ons, $(SRC_DIR_IN))

