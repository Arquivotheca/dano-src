SRC_DIR:=src/kit/controls/columnlistview

TARGET_NAME := libcolumnlistview.a

SRCS :=  \
	ColumnListView.cpp \
	ColumnTypes.cpp

MY_OBJ_OFFSET := static.f

MY_CFLAGS := $(F_NO_EXCEPTION) -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1 \
	$(call MFLAG_TO_DFLAG, NETRON_MODE) \
#

MY_DFLAGS := \
	-D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1 \
#

MY_INCLUDES := \
		-I$(BUILDHOME)/headers/controls \
		-I$(BUILDHOME)/src/inc/app_p \
		-I$(BUILDHOME)/src/inc/interface_p \
		-I$(BUILDHOME)/src/inc/support_p

MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)
LINK_DIRS :=

SYSTEM_LIBS := libexperimental.a

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk



SRC_DIR:=src/kit/controls/columnlistview

TARGET_NAME := libcolumnlistview.so

SRCS :=  \
	ColumnListView.cpp \
	ColumnTypes.cpp

MY_OBJ_OFFSET := shared.f

MY_CFLAGS := $(F_NO_EXCEPTION) -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := \
		-I$(BUILDHOME)/headers/controls \
		-I$(BUILDHOME)/src/inc/app_p \
		-I$(BUILDHOME)/src/inc/interface_p \
		-I$(BUILDHOME)/src/inc/support_p

MY_INSTALL_DIR := $(INSTALL_LIBS)
LINK_DIRS :=

SYSTEM_LIBS := libroot.so libbe.so libexperimental.a

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

