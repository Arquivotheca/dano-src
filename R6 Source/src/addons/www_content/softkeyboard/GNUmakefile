# Makefile

TARGET_NAME:=softkeyboard.so

SRC_DIR:=src/addons/www_content/softkeyboard

SRCS := \
		SoftKeyboardContent.cpp			\
		SoftKeyboard.cpp				\
		Key.cpp

MY_CFLAGS := $(F_NO_EXCEPTIONS)

MY_INCLUDES:= -I$(BUILDHOME)/headers/www -I${BUILDHOME}/srcx/gnu/libpng

SYSTEM_LIBS:= libroot.so libbe.so libwww.so libexperimental.a libpng.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "application/x-vnd.Be.SoftKeyboard"	\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
