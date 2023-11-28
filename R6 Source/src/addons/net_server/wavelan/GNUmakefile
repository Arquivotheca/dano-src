# Net_Server common
SRC_DIR:=src/addons/net_server
MY_OBJ_OFFSET := wavelan.f
SRCS		+= \
			generic.cpp \
			EtherDevice.cpp
#
TYPE := SHARED

MY_CFLAGS := -DUSE_OFFS=0 $(FORCE_EXCEPTIONS)

MY_INCLUDES:= \
			-I$(BUILDHOME)/src/inc/support_p \
			-I$(BUILDHOME)/src/inc/net_p \
			-I$(BUILDHOME)/src/inc/driver \
			-I$(call FULL_SRC_DIR, src/addons/net_server/wavelan)/common

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
MORE_OBJS := $(OBJS_OUT)

MY_INCLUDES := $(MY_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)

# Wavelan driver
TARGET_NAME:=wavelan

SRC_DIR:=src/addons/net_server/wavelan

SRCS		:= wavelan.cpp

SYSTEM_LIBS:=libroot.so libbe.so libnetdev.so

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/net_server

SIGNATURE:= "application/x-vnd.Be-wavelan"


include $(SDIR_TEMPLATES)/AddonTemplate.mk
