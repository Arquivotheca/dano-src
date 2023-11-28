SRC_DIR:=src/kit/media
MY_OBJ_OFFSET := avi.writer.f
SRCS :=  \
	FileWriter.cpp
#
TYPE := SHARED

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p \
	-I$(BUILDHOME)/src/addons/media/extractors/avi/

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

MY_INCLUDES := $(MY_INCLUDES_IN)


TARGET_NAME:=avi.writer

SRC_DIR:=src/addons/media/writers/avi

SRCS := AVI.cpp         \
        AVIUtils.cpp    \
        TRIFFWriter.cpp	\
#

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/writers

include $(SDIR_TEMPLATES)/AddonTemplate.mk

