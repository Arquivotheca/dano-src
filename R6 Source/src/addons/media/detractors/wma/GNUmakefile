TARGET_NAME:=wma.detractor

SRC_DIR:=src/addons/media/detractors/wma

SRCS := WMADetractor.cpp WMAGlue.cpp

MY_CFLAGS :=

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

MORE_OBJS := WMA_Dec_Emb_x86_full.a wma_dec_Emb_x86_drm.a

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/detractors

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
