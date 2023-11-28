# Makefile

TARGET_NAME:=midi_rmf.so

SRC_DIR:=src/addons/www_content/midi_rmf

SRCS := MIDIRMF.cpp

MY_CFLAGS :=

MY_INCLUDES:= -I$(BUILDHOME)/headers/www -I$(BUILDHOME)/src/kit/bae

SYSTEM_LIBS:= libroot.so libbe.so libwww.so libbae.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "audio/midi"				\
			"mime_types" "audio/x-midi"				\
			"extensions" "midi"						\
			"extensions" "mid"						\
			"mime_types" "audio/rmf"				\
			"mime_types" "audio/x-rmf"				\
			"extensions" "rmf"						\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
