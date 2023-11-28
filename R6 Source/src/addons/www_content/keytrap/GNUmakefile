# Makefile

TARGET_NAME:=keytrap.so

SRC_DIR:=src/addons/www_content/keytrap

SRCS := KeyTrapContent.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/headers/www \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(BUILDHOME)/src/inc/support_p \
	-I$(BUILDHOME)/src/inc/tracker_p

SYSTEM_LIBS:= libroot.so libbe.so libwww.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "application/x-vnd.Be.KeyTrap"	\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=KeyTrap.rsrc


include $(SDIR_TEMPLATES)/AddonTemplate.mk
