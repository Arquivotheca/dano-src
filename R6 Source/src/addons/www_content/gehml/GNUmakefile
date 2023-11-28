# Makefile

TARGET_NAME:=gehml.so

SRC_DIR:=src/addons/www_content/gehml

SRCS := \
	GroupNamespace.cpp \
	GehmlPortal.cpp \
	BDrawable.cpp \
	BAppSessionDrawable.cpp \
	BViewDrawable.cpp \
	DirtyHacks.cpp \
	GehmlBlock.cpp \
	GehmlContent.cpp \
	GehmlRoot.cpp \
	GehmlDocument.cpp \
	GehmlEmbeddedRoot.cpp \
	GehmlGroup.cpp  \
	GehmlHVGroup.cpp  \
	GehmlLayout.cpp  \
	GehmlObject.cpp \
	GehmlStack.cpp \
	GehmlUtils.cpp \
	GehmlWindow.cpp \
#

MY_CFLAGS := 
MY_LDFLAGS := /boot/home/config/lib/libbe.so

MY_INCLUDES:= \
	-I. \
	-I$(BUILDHOME)/headers/www \
	-I$(BUILDHOME)/headers/support \
	-I$(BUILDHOME)/headers/xml \
	-I$(BUILDHOME)/src/inc \
	-I$(BUILDHOME)/src/inc/app_p \
	-I$(BUILDHOME)/src/inc/app_server_p \
#

SYSTEM_LIBS:= libroot.so libwww.so libxml.so

CC :=        $(BUILDHOME)/gnupro-000224/bin/gcc
CXX =       $(BUILDHOME)/gnupro-000224/bin/g++

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "image/x-gehml"			\
			"extensions" "gml"						\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=

MY_OPTIMIZER:= -O3
include $(SDIR_TEMPLATES)/AddonTemplate.mk
