TARGET_NAME:=validate

SRC_DIR:= src/apps/$(TARGET_NAME)

SRCS := md5/md5_dgst.c \
	Test.cpp \
	ValidApp.cpp \
	acceptance.cpp \
	ethernet.cpp \
	file_read.cpp \
	file_write.cpp \
	keyboard_key.cpp \
	modem.cpp \
	mouse_click.cpp \
	SoundCheckWindow.cpp \
	sound_confirmation.cpp \
	validate.cpp \
	settings.cpp \
	vendor_tests.cpp

MY_INSTALL_DIR:=$(INSTALL_VALIDATE)
EXCLUDES_FILE_NAME := checksum-exclusions.txt
MY_CFLAGS := -DMAKE_HORRID_BEEP_ON_FAILURE=1 -DEXCLUDES_FILE_NAME=\"$(EXCLUDES_FILE_NAME)\"

MY_INCLUDES := \
	-I$(BUILDHOME)/$(SRC_DIR)/md5	\
	-I$(BUILDHOME)/src/inc/media_p

SYSTEM_LIBS:= libroot.so libsocket.so libbe.so libmedia.so

RESOURCE_FORK:= 

LINK_DIRS := $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)


include $(SDIR_TEMPLATES)/AppTemplate.mk

# The Sony sound files (only specified in SONY.spec)

SOURCE:= SonyTestFile07.mp3
SRC_DIR := src/apps/validate
TARGET := $(INSTALL_VALIDATE)/SonyTestFile07.mp3
PERMISSIONS := $(PERMS_USER_DATA)
ATTRIBUTES :=
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE:= 1_5_khz1s.mp3
SRC_DIR := src/apps/validate
TARGET := $(INSTALL_VALIDATE)/1_5_khz1s.mp3
PERMISSIONS := $(PERMS_USER_DATA)
ATTRIBUTES :=
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE:= 1_5_khz2s.mp3
SRC_DIR := src/apps/validate
TARGET := $(INSTALL_VALIDATE)/1_5_khz2s.mp3
PERMISSIONS := $(PERMS_USER_DATA)
ATTRIBUTES :=
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE:= 1_5_khz3s.mp3
SRC_DIR := src/apps/validate
TARGET := $(INSTALL_VALIDATE)/1_5_khz3s.mp3
PERMISSIONS := $(PERMS_USER_DATA)
ATTRIBUTES :=
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE:= one_khz1s.mp3
SRC_DIR := src/apps/validate
TARGET := $(INSTALL_VALIDATE)/one_khz1s.mp3
PERMISSIONS := $(PERMS_USER_DATA)
ATTRIBUTES :=
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

SOURCE:= one_khz2s.mp3
SRC_DIR := src/apps/validate
TARGET := $(INSTALL_VALIDATE)/one_khz2s.mp3
PERMISSIONS := $(PERMS_USER_DATA)
ATTRIBUTES :=
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

# The validate.ini settings file

SRC_DIR := src/apps/validate
SOURCE := $(call MVAR_TO_TARGET_NAME,VALIDATE_VALIDATE_INI,validate.ini)
TARGET := $(INSTALL_VALIDATE)/validate.ini
PERMISSIONS := 0755
ATTRIBUTES :=

MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

# The md5 checksum exclusions file list file.
SRC_DIR := src/apps/validate
SOURCE := $(EXCLUDES_FILE_NAME)
TARGET := $(INSTALL_VALIDATE)/$(EXCLUDES_FILE_NAME)
PERMISSIONS := 0755
ATTRIBUTES :=


MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

ifneq ($(call MVAR_EXISTS,VALIDATE_ADDONS),)
-include $(BUILDHOME)/src/apps/validate/$(call MVAR_TO_TARGET_NAME,VALIDATE_ADDONS,)/GNUmakefile
endif

SRC_DIR := src/apps/validate
SRCS := mictest.cpp
MY_INSTALL_DIR := $(INSTALL_VALIDATE)
SYSTEM_LIBS := libroot.so libbe.so libmedia.so
TARGET_NAME := mictest
MY_INCLUDES := 	-I$(BUILDHOME)/src/inc/media_p
include $(SDIR_TEMPLATES)/AppTemplate.mk
