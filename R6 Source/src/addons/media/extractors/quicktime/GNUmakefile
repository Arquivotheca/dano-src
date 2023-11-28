TARGET_NAME:=quicktime.extractor

SRC_DIR:=src/addons/media/extractors/quicktime

SRCS := QTAtomHandler.cpp  \
        QTTrack.cpp        \
        QuickTime.cpp

MY_DFLAGS := 

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(BUILDHOME)/srcx/gnu/zlib-1.1.3 \
#

SYSTEM_LIBS:= libroot.so libbe.so libz.a libmedia.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/extractors

include $(SDIR_TEMPLATES)/AddonTemplate.mk
