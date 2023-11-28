# Makefile

TARGET_NAME:=fileprotocol.so

SRC_DIR:=src/addons/www_protocol2/file

SRCS := FileProtocol.cpp

MY_CFLAGS :=

MY_INCLUDES:= -I$(BUILDHOME)/headers/www2

SYSTEM_LIBS:= libroot.so libbe2.so libwww2.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"schemes" "file"						\
			-t bool "be:protocol" true				\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
