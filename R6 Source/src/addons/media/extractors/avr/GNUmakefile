TARGET_NAME:=avr.extractor

SRC_DIR:=src/addons/media/extractors/avr

SRCS := avr_extractor.cpp

MY_DFLAGS := -DNDEBUG=1

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p 

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/extractors

include $(SDIR_TEMPLATES)/AddonTemplate.mk
