# Media Kit Code
SRC_DIR:=src/kit/media
MY_OBJ_OFFSET := quicktime.writer.f
SRCS :=  \
	FileWriter.cpp
#
TYPE := SHARED

MY_INCLUDES:= -I$(call FULL_SRC_DIR, src/addons/media/writers/quicktime) \
	-I$(call FULL_SRC_DIR, src/addons/media/extractors/quicktime) \
	-I$(call FULL_SRC_DIR, src/kit/media) \
	-I$(BUILDHOME)/src/inc/media_p

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

MY_INCLUDES := $(MY_INCLUDES_IN)

# Extractor code
SRC_DIR:=src/addons/media/extractors/quicktime
MY_OBJ_OFFSET := quicktime.writer.f
SRCS :=  \
        QTTrack.cpp \
#
TYPE := SHARED

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS += $(OBJS_OUT)

MY_INCLUDES := $(MY_INCLUDES_IN)

# Writer code
TARGET_NAME:=quicktime.writer

SRC_DIR:=src/addons/media/writers/quicktime

SRCS := QTWriter.cpp       \
        QTAtomWriter.cpp   \
#

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/writers

include $(SDIR_TEMPLATES)/AddonTemplate.mk

