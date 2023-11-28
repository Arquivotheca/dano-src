# Makefile

TARGET_NAME:=japaneseim.so

SRC_DIR:=src/addons/www_content/japanese_im

SRCS := JapaneseIMContent.cpp		\
	ModeButton.cpp			\
	BitmapData.cpp			\
#

MY_CFLAGS := $(F_NO_EXCEPTIONS)

MY_INCLUDES:= -I$(BUILDHOME)/headers/www -I$(BUILDHOME)/src/addons/input_server/methods/japanese

SYSTEM_LIBS:= libroot.so libbe.so libwww.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "application/x-vnd.Be.JapaneseIM"	\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
