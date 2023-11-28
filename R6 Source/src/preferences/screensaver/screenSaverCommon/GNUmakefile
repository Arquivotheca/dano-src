# screen saver library

TARGET_NAME:=screensaverlib.a

SRC_DIR := src/preferences/screensaver/screenSaverCommon

SRCS := \
	ssdefs.cpp \
	ScreenSaver.cpp \
	Blackness.cpp \
#

MY_CFLAGS := -DSCREENSAVERS_DIR_LEAF_NAME=\"${notdir \
	${INSTALL_SCREEN_SAVERS}}\"
MY_INCLUDES:= -I$(BUILDHOME)/headers/add-ons/screen_saver

SYSTEM_LIBS :=
MY_INSTALL_DIR :=

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

