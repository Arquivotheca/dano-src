# Makefile

TARGET_NAME:=videoplayer.so

SRC_DIR:=src/addons/www_content/videoplayer

SRCS := VideoPlayer.cpp VideoDecoder.cpp ForwardIO.cpp VideoView.cpp

MY_CFLAGS:=

ifeq ($(call MVAR_TO_TARGET_NAME, VIDEO_PLAYERS_BACKGROUND, default),netron)
MY_CFLAGS+=-DSONY_MPEG_BACKGROUND=1
endif

MY_INCLUDES:= -I$(BUILDHOME)/headers/www

SYSTEM_LIBS:= libroot.so libbe.so libwww.so libmedia.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "video/mpeg"				\
			"mime_types" "video/x-mpeg"				\
			"extensions" "mpeg"						\
			"extensions" "mpg"						\
			"extensions" "mpgs"						\
			"extensions" "mpv"						\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
