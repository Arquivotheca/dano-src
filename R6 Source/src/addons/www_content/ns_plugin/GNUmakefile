# Makefile

TARGET_NAME:=ns_plugin.so

SRC_DIR:=src/addons/www_content/ns_plugin

SRCS := NetscapePlugin.cpp NPN_funcs.cpp 

MY_CFLAGS :=

MY_INCLUDES:= -I$(BUILDHOME)/headers/www

SYSTEM_LIBS:= libroot.so libbe.so libwww.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0				\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
