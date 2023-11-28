# Makefile

TARGET_NAME:=mediaplayer.so

SRC_DIR:=src/addons/www_content/mediaplayer2

SRCS := MediaContent.cpp PlaybackEngine.cpp IOAdapter.cpp MidiHandler.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/headers/www \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(BUILDHOME)/src/inc/support_p \
	-I$(BUILDHOME)/src/inc/tracker_p \
	-I$(BUILDHOME)/src/kit/bae

SYSTEM_LIBS:= libroot.so libbe.so libwww.so libmedia.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "audio/mid"				\
			"mime_types" "audio/midi"				\
			"mime_types" "audio/x-midi"				\
			"extensions" "midi"						\
			"extensions" "mid"						\
			"mime_types" "audio/rmf"				\
			"mime_types" "audio/x-rmf"				\
			"extensions" "rmf"						\
			"mime_types" "audio/x-aiff"				\
			"mime_types" "audio/aiff"				\
			"extensions" "aiff"						\
			"mime_types" "audio/wav"				\
			"mime_types" "audio/x-wav"				\
			"extensions" "wav"						\
			"mime_types" "audio/mpeg"				\
			"mime_types" "audio/x-mpeg"				\
			"extensions" "mp3"						\
			"mime_types" "audio/x-mpegurl"			\
			"extensions" "m3u"						\
			"mime_types" "audio/x-scpls"			\
			"extensions" "pls"						\
			"mime_types" "audio/basic"				\
			"extensions" "au"						\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
