TARGET_NAME:=mpeg2.extractor

SRC_DIR:=src/addons/media/extractors/mpeg2

SRCS := mpeg2.extractor.cpp \
	BitStream.cpp \
	PositionIOBitStream.cpp \
	MemoryBitStream.cpp \
	MPEG2Track.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p 

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/extractors

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
