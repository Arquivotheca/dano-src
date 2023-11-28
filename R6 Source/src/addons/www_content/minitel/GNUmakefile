# Makefile

#
# THIS DOES NOT HANDLE ADDING MINITEL TO THE FAVORITES!
# I don't think that should be handled here. (steven 2000-08-09)
#

TARGET_NAME:=minitel.so

SRC_DIR:=src/addons/www_content/minitel

SRCS			:=						\
				MinitelContent.cpp		\
				MinitelView.cpp			\
				Iso6429ScreenModule.cpp	\
				KeyboardModule.cpp		\
				ModemModule.cpp			\
				Protocole.cpp			\
				StdScreenModule.cpp		\
				VirtScreenModule.cpp

MY_CFLAGS :=

MY_INCLUDES:= -I$(BUILDHOME)/headers/www -I$(BUILDHOME)/src/inc/www_p

SYSTEM_LIBS:= libroot.so libbe.so libwww.so libdevice.so libsettings.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "application/x-minitel"	\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=Minitel.rsrc


include $(SDIR_TEMPLATES)/AddonTemplate.mk

SRC_DIR:=src/addons/www_content/minitel
FILES:=Minitel_40.html
MY_INSTALL_DIR:= $(INSTALL_MINITEL)

PERMISSIONS:=$(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataTemplate.mk

SRC_DIR:=src/addons/www_content/minitel
SOURCE:=minitel_page
TARGET:= $(INSTALL_MINITEL)/$(SOURCE)
ATTRIBUTES:=-complex raw -t time 0 -t message		\
			"mime_types" "application/x-minitel"	\
			-end									\
			-name "BEOS:TYPE"

PERMISSIONS:=$(PERMS_SYSTEM_DATA)
MY_DEPS:=$(MAKEFILE_LEAFNAME)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

