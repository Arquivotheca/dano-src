# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000; Be, Inc. All Rights Reserved.
#
# REMEMBER: You can't comment out any of these, though some of them can be
#  set to empty. (The flat makefile system means if you don't set things to
#  empty, they likely won't be empty when you're called.)

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libergokanakan.a

# SRC_DIR - the relative path from $(BUILDHOME) of the root of your
#           application source tree. (can not be empty)
SRC_DIR:=src/addons/input_server/methods/japanese/Analysis

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := AccessTable/AccessTable.cpp \
		AccessTable/FieldStruct.cpp \
		AccessTable/HinshiStruct.cpp \
		AccessTable/HinshiTableArray.cpp \
		AccessTable/NumberHyoki.cpp \
		Analyze/Analysis.cpp \
		DicAccess/DicAccess.cpp \
		DicCtrl/DicCtrl.cpp \
		DicCtrl/ExtraDicCtrl.cpp \
		DicData/DicData.cpp \
		DoubleArray/DAadd.c \
		DoubleArray/DAcache.c \
		DoubleArray/DAcreate.c \
		DoubleArray/DAdelete.c \
		DoubleArray/DAdump.c \
		DoubleArray/DAfile.c \
		DoubleArray/DAsearch.c \
		DoubleArray/DoubleArray.c \
		Grammar/Grammar.cpp \
		Grammar/gramDisp.cpp

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := $(FORCE_CPP)

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(BUILDHOME)/$(SRC_DIR) \
				-I$(BUILDHOME)/$(SRC_DIR)/AccessTable \
				-I$(BUILDHOME)/$(SRC_DIR)/Analyze \
				-I$(BUILDHOME)/$(SRC_DIR)/DicAccess \
				-I$(BUILDHOME)/$(SRC_DIR)/DicCtrl \
				-I$(BUILDHOME)/$(SRC_DIR)/DicData \
				-I$(BUILDHOME)/$(SRC_DIR)/DoubleArray \
				-I$(BUILDHOME)/$(SRC_DIR)/Grammar
				

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.a
LINKS :=


include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:=yomi.dic
TARGET:=$(INSTALL_ETC)/yomi.dic
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
SOURCE:=user.dic
TARGET:=$(INSTALL_USER_SETTINGS)/user.dic
include $(SDIR_TEMPLATES)/DataFileTemplate.mk
