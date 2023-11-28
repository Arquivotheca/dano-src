# Makefile

TARGET_NAME:=beosprotocol.so

SRC_DIR:=src/addons/www_protocol2/beos

SRCS := BeOSProtocol.cpp

MY_CFLAGS := $(F_NO_EXCEPTIONS)

MY_INCLUDES:= -I$(BUILDHOME)/headers/www -I$(BUILDHOME)/src/inc/www_p

SYSTEM_LIBS:= libroot.so libbe2.so libwww2.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

# Install the identifier attributes for this add-on.
# BE AWARE: Any changes to these attributes should also
# be made to the factory class.
# Also note that we set a modification time of 0, so
# the attributes will never be overwritten from the
# executable, even if it is later changed.
ATTRIBUTES := -complex raw -t time 0 -t message		\
			"schemes" "beos"						\
			-t bool "be:protocol" true				\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
