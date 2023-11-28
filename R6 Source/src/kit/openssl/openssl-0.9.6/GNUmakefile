
OPENSSL_CRYPTO_MAKEFILE:=$(call FULL_SRC_DIR, src/kit/openssl/openssl-0.9.6)/crypto/$(MAKEFILE_LEAFNAME)
ifeq ($(wildcard $(OPENSSL_CRYPTO_MAKEFILE)),)
FOO:=$(shell ${PTOOLS}/p4 sync -f $(OPENSSL_CRYPTO_MAKEFILE))
ifeq ($(shell test -f $(OPENSSL_CRYPTO_MAKEFILE) || echo failed),)
$(error Your SSL issue has been fixed. Please restart make)
else
$(error Could not resync '$(OPENSSL_CRYPTO_MAKEFILE)' to fix your SSL issue. Please do so manually and restart make)
endif
endif

TARGET_NAME := libopenssl.so

TYPE := SHARED

SRC_DIR := src/kit/openssl/openssl-0.9.6

MY_DIRS := crypto ssl

SYSTEM_LIBS := libroot.so libbe.so libbind.so libsocket.so \
#

OUR_INCLUDES := \
	-I$(call FULL_SRC_DIR, $(SRC_DIR))/crypto \
#

OUR_DFLAGS :=  -D_BUILDING_openssl=1 \
	-DTHREADS -D_REENTRANT -DNO_RC5 -DNO_CAST \
	-DNO_DH -DNO_IDEA -DL_ENDIAN

MY_INSTALL_DIR := $(INSTALL_LIBS)

include $(SDIR_TEMPLATES)/ComplexTemplate.mk

# absolute link to /boot/beos/system/lib
SOURCE	:= /boot/beos/system/lib/libopenssl.so

SRC_DIR := src/kit/openssl/openssl-0.9.6

LINK_PREFIX		:=
LINK_RELATION	:=
LINKS			:=$(INSTALL_DEV_LIBS)/libopenssl.so
MY_DEPS			:=$(MAKEFILE_LEAFNAME)

include $(SDIR_TEMPLATES)/SymlinkTemplate.mk

# Makefile using GenericGeneratedTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the installed created file (can not be empty)
TARGET_NAME := buildinf.h

# MY_RULE - the name of the rule to use -- Remember the rule won't have
#  it's variables expanded when it's defined. Rules should be defined in
#  Macros.mk and end in _RULE. (can not be empty)
MY_RULE := OPENSSL_BUILDINF_RULE

# MY_DEPS - the list of fully qualified items to depend upon
MY_DEPS := $(COMPLEX_SRCS_OUT)

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
# NOTE: Either SRC_DIR or MY_INSTALL_DIR need to be defined. The target
#  will be in the OBJ_DIR if MY_INSTALL_DIR is undefined.
SRC_DIR := src/kit/openssl/openssl-0.9.6

# MY_INSTALL_DIR - the absolute destination directory -- should be an
#  INSTALL_ constant. (defaults to OBJ_DIR) If it defaults to the OBJ_DIR
#  it's treated as a build item and not an installed item. (It's attached
#  to the build vs. install rules.)
# NOTE: Either SRC_DIR or MY_INSTALL_DIR need to be defined. The target
#  will be in the OBJ_DIR if MY_INSTALL_DIR is undefined.
MY_INSTALL_DIR :=

include $(SDIR_TEMPLATES)/GenericGeneratedTemplate.mk

# Makefile using DataTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/kit/openssl/openssl-0.9.6/include/openssl

#  FILES - the name of the files to copy. (can not be empty)
FILES := $(call ALL_DOT_WHAT, h)

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. (can not be empty)
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# Note: if you need MY_INSTALL_DIR to be empty, you don't need this template
MY_INSTALL_DIR := $(INSTALL_DEV_OPENSSL_HEADERS)

# dereference any symlinks encountered
COPY_AS_FILES := true

# PERMISSIONS - the appropriate PERMS_* setting (can not be empty)
PERMISSIONS := $(PERMS_SYSTEM_DATA)

include $(SDIR_TEMPLATES)/DataTemplate.mk

