
TARGET_NAME:=libbe2.so

TYPE := SHARED

SRC_DIR:= src/kit/libbe2

MY_DIRS := $(call FULL_NAMES, \
	content2 \
	interface2 \
	raster2 \
	render2 \
	storage2 \
	support2 \
, $(SRC_BUILDHOME)/src/kit) \
	old \
#

MY_INSTALL_DIR:= $(INSTALL_LIBS)

OUR_CFLAGS := \
			-DBINDER_DEBUG_LIB=1 \
			-DBINDER_DEBUG_MSGS=0 \
			$(call MFLAG_TO_DFLAG, SUPPORTS_ATOM_DEBUG) \
			$(call MFLAG_TO_DFLAG, SUPPORTS_LOCK_DEBUG) \
#

SYSTEM_LIBS := libroot.so libbinderdbg.so
ifneq ($(call MFLAG_IS_SET, SUPPORTS_APP_SERVER_FEATURE_REMOTE_DISPLAY),)
	SYSTEM_LIBS	+=	libsocket.so libbind.so
endif
ifneq ($(call MFLAG_IS_SET, SUPPORTS_APP_SERVER_FEATURE_SYNC_CALL_LOGGING),)
	SYSTEM_LIBS	+=	libdevel.so
endif


LINKS := $(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/ComplexTemplate.mk
