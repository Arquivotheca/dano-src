SRC_DIR:=src/kit/media
MY_OBJ_OFFSET := wav.writer.f
SRCS :=  \
	FileWriter.cpp
#
TYPE := SHARED

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

MY_INCLUDES := $(MY_INCLUDES_IN)


TARGET_NAME:=wav.writer

SRC_DIR:=src/addons/media/writers/wav

SRCS := wav.cpp

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/writers

include $(SDIR_TEMPLATES)/AddonTemplate.mk

