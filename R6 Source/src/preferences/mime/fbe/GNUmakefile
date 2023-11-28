SRC_DIR:=src/preferences/mime/fbe

TARGET_NAME := fbelib.a

SRCS := \
		BitsContainer.cpp \
		ColorPicker.cpp \
		FatBitsEditor.cpp \
		IconBits.cpp \
		IconEditor.cpp \
		IconEditView.cpp \
		IconPicker.cpp \
		IconView.cpp \
		IconWindow.cpp \
		ToolPicker.cpp \
		utils.cpp \
#

MY_CFLAGS := -DUSE_OFFS=0

MY_INCLUDES := -I$(BUILDHOME)/$(SRC_DIR) \
	-I$(BUILDHOME)/src/inc/os_p \
	-I$(BUILDHOME)/src/inc/interface_p


# Hackery to be able to see the libpng header files.
MY_INCLUDES	+= -I${BUILDHOME}/srcx/gnu/libpng

MY_INSTALL_DIR :=

SYSTEM_LIBS :=

LINK_DIRS :=

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk
