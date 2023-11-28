# GNUmakefile for pulsecontent.so
######################################################################

TARGET_NAME:=pulsecontent.so

SRC_DIR:=src/addons/www_content/pulse

SRCS := PulseContentInstance.cpp	\
		PulseContent.cpp
#

MY_CFLAGS := $(F_NO_EXCEPTIONS)

MY_INCLUDES:= -I$(BUILDHOME)/headers/www

SYSTEM_LIBS:= libroot.so libbe.so libwww.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "application/x-vnd.Be.PulseContent"	\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

######################################################################
# end of GNUmakefile for pulsecontent
