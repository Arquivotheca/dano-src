# Makefile

TARGET_NAME:=realprotocol.so

SRC_DIR:=src/addons/www_protocol2/real

SRCS := RealProtocol.cpp

MY_CFLAGS :=

MY_INCLUDES:=	-I$(BUILDHOME)/headers/support2 \
				-I$(BUILDHOME)/headers/www2 \

SYSTEM_LIBS:= libroot.so libbe2.so libwww2.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"schemes" "pnm"							\
			"schemes" "rtsp"						\
			-t bool "be:protocol" true				\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
