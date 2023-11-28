TARGET_NAME:=avi.extractor

SRC_DIR:=src/addons/media/extractors/avi

SRCS := AVI.cpp         \
        AVIUtils.cpp    \
        TRIFFReader.cpp

MY_DFLAGS := -DNDEBUG=1

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc/media_p 

SYSTEM_LIBS:= libroot.so libbe.so libmedia.so

ifeq ($(call MVAR_TO_TARGET_NAME, COMPILER_VERSION, gcc2),gcc3) 
SYSTEM_LIBS += libsupc++.a
endif

$(call MVAR_CHECK_FINITE_OPTIONS, COMPILER_VERSION, gcc2 gcc3, gcc2)

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/media/extractors

include $(SDIR_TEMPLATES)/AddonTemplate.mk
