SRC_DIR:=src/kit/reseditor

TARGET_NAME := libreseditor.so

SRCS :=  \
	Ref.cpp \
	ResourceAddon.cpp \
	ResourceEditor.cpp \
	ResourceEntry.cpp \
	ResourceItem.cpp \
	ResourceListView.cpp \
	ResourceRoster.cpp \
	ToolTipHook.cpp \
	UndoContext.cpp \
#

MY_DFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

MY_INCLUDES := \
	-I$(BUILDHOME)/headers/reseditor -I$(BUILDHOME)/src/inc/reseditor_p
#

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libbe.so \
	libtranslation.so libexperimental.a libcolumnlistview.a

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

SRC_DIR:=$(SRC_DIR_IN)
TARGET_NAME := $(TARGET_NAME_IN)
SRCS :=  $(SRCS_IN)
MY_DFLAGS :=  $(MY_DFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)
SYSTEM_LIBS := $(SYSTEM_LIBS_IN) libstdc++.r4.so

include $(SDIR_TEMPLATES)/HostSharedLibTemplate.mk


