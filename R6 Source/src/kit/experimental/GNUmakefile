SRC_DIR:=src/kit/experimental

TARGET_NAME := libexperimental.a

MY_DFLAGS := -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1

SRCS		:= \
	BitmapButton.cpp \
	BitmapTools.cpp \
	ColorTools.cpp \
	MultiLocker.cpp \
	Order.cpp \
	ResourceSet.cpp \
#

# Don't build these under IAD --  they aren't needed.
ifneq ($(call MFLAG_IS_SET, SUPPORTS_ALL_EXPERIMENTAL_CLASSES),)

SRCS		+= \
	DividerControl.cpp \
	DocApplication.cpp \
	DocWindow.cpp \
	ObjectList.cpp \

endif

MY_DFLAGS += $(call MFLAG_TO_DFLAG, NETRON_MODE)

MY_DFLAGS += $(call MFLAG_TO_DFLAG, LIBEXPERIMENTAL_AUTO_BITMAP_GENERATION, AUTO_BITMAP_GENERATION)

MY_INCLUDES := -I$(BUILDHOME)/src/inc/app_p \
			-I$(BUILDHOME)/src/inc/interface_p \
			-I$(BUILDHOME)/src/inc/support_p \
#

# Hackery to be able to see the libpng header files.
MY_INCLUDES	+= -I${BUILDHOME}/srcx/gnu/libpng \
			-I$(BUILDHOME)/srcx/gnu/zlib-1.1.3 \
#

MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

