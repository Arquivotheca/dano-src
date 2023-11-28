# Makefile

TARGET_NAME:=updater.so

SRC_DIR:=src/addons/www_content/update

SRCS := updater.cpp

MY_CFLAGS :=

MY_INCLUDES:= -I$(BUILDHOME)/headers/www

SYSTEM_LIBS:= libroot.so libbe.so libwww.so

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
			"mime_types" "application/x-vnd.be-iaupdater"				\
			"extensions" "upd"						\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
