TARGET_NAME:=ssl.so

SRC_DIR:=src/addons/www_protocol/ssl

SRCS :=	interface.c

MY_CFLAGS :=

MY_CFLAGS += $(call MFLAG_TO_DFLAG, SSL_STRONG_ENCRYPTION)

MY_INCLUDES:= -I$(BUILDHOME)/headers/www

SYSTEM_LIBS:= libroot.so libbe.so libwww.so libopenssl.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0				\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
