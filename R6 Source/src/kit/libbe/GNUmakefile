LOCAL_RESOURCES := 
ifneq ($(call MFLAG_IS_SET, SUPPORTS_BEOS_ABOUT_BOX),)
LOCAL_RESOURCES += $(call FULL_SRC_DIR, src/kit/interface)/AboutBoxIcons.rsrc
endif
ifneq ($(call MFLAG_IS_SET, PRINTING_FOR_DESKTOP),)
LOCAL_RESOURCES += $(call FULL_SRC_DIR, src/kit/print)/print_bitmaps.rsrc		
endif

LOCAL_LIBS := libroot.so libtextencoding.so libpng.so

ifeq ($(call MVAR_TO_TARGET_NAME, COMPILER_VERSION, gcc2),gcc3) 
LOCAL_LIBS += libstdc++.a
endif

$(call MVAR_CHECK_FINITE_OPTIONS, COMPILER_VERSION, gcc2 gcc3, gcc2)

ifneq ($(call MFLAG_IS_SET, SUPPORTS_APP_SERVER_FEATURE_REMOTE_DISPLAY),)
	LOCAL_LIBS	+=	libsocket.so libbind.so
endif
ifneq ($(call MFLAG_IS_SET, SUPPORTS_APP_SERVER_FEATURE_SYNC_CALL_LOGGING),)
	LOCAL_LIBS	+=	libdevel.so
endif

LOCAL_DIRS := $(call FULL_NAMES, \
	app interface storage support print \
, $(SRC_BUILDHOME)/src/kit)


TARGET_NAME:=libbe.so
TYPE := SHARED
SRC_DIR:= src/kit/libbe
MY_DIRS := $(LOCAL_DIRS)
MY_INSTALL_DIR:= $(INSTALL_LIBS)
OUR_CFLAGS := \
			$(call MFLAG_TO_DFLAG, SUPPORTS_STREAM_IO) \
			$(call MFLAG_TO_DFLAG, SUPPORTS_ATOM_DEBUG) \
			$(call MFLAG_TO_DFLAG, SUPPORTS_LOCK_DEBUG) \
			$(call MFLAG_TO_DFLAG, SUPPORTS_OLD_ATOM_DEBUG) \
#
SYSTEM_LIBS := $(LOCAL_LIBS)
RESOURCES := $(LOCAL_RESOURCES)
LINKS := $(INSTALL_DEV_LIBS)/$(TARGET_NAME)
include $(SDIR_TEMPLATES)/ComplexTemplate.mk

ifneq ($(call MFLAG_IS_SET, SUPPORTS_BEOS_ABOUT_BOX),)
TARGET_NAME:=libbe.so.rsrc
RESOURCES := $(LOCAL_RESOURCES)
SRC_DIR:= src/kit/libbe
MY_INSTALL_DIR:= $(INSTALL_LIBS)
include $(SDIR_TEMPLATES)/ResourceTemplate.mk
endif


# Whenever you set OVERRIDE_MFLAGS, you always need to clear
# it when you are done.
OVERRIDE_MFLAGS := \
		SUPPORTS_STREAM_IO \
		SUPPORTS_ATOM_DEBUG \
		SUPPORTS_OLD_ATOM_DEBUG \
		SUPPORTS_LOCK_DEBUG \
#

TARGET_NAME:=libbe-debug.so
OVERRIDE_SONAME:=libbe.so
TYPE := SHARED
OUR_OBJ_OFFSET := $(TARGET_NAME).f
SRC_DIR:= src/kit/libbe
MY_DIRS := $(LOCAL_DIRS)
MY_INSTALL_DIR:= $(INSTALL_LIBS)
OUR_DFLAGS :=
OUR_CFLAGS := \
			-DSUPPORTS_STREAM_IO=1 \
			-DSUPPORTS_ATOM_DEBUG=1 \
			-DSUPPORTS_LOCK_DEBUG=1 \
			-DSUPPORTS_OLD_ATOM_DEBUG=1 \
#
SYSTEM_LIBS := $(LOCAL_LIBS)
RESOURCES := $(LOCAL_RESOURCES)
include $(SDIR_TEMPLATES)/ComplexTemplate.mk
# Whenever you set OVERRIDE_MFLAGS, you always need to clear
# it when you are done.
OVERRIDE_MFLAGS :=

SOURCE:=$(CREATED_LIBRARY_PATH)/libbe-debug.so
SRC_DIR :=
TARGET := $(INSTALL_USER_BASE)/libbe.so
ELF_FILES += $(TARGET)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk
