# Makefile

TARGET_NAME:=lbx.so

SRC_DIR:=src/addons/www_content/lbx

SRCS := Lbx.cpp

MY_CFLAGS :=

SYSTEM_LIBS:= libroot.so libbe.so libwww.so
MY_INCLUDES:= -I$(BUILDHOME)/headers/www	\
	-I$(BUILDHOME)/src/inc/support_p	\
#

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "application/x-vnd.Be.Lbx" \
			-end									\
			-name "be:content"


include $(SDIR_TEMPLATES)/AddonTemplate.mk
