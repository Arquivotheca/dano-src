TARGET_NAME:=japanese

SRC_DIR:=src/addons/input_server/methods/$(TARGET_NAME)

SRCS := Japanese.cpp \
		BitmapData.cpp \
		HenkanManager.cpp \
		KanaKan.cpp \
		KanaString.cpp \
		KouhoWindow.cpp \
		ModePalette.cpp

MY_CFLAGS := -DUSE_OFFS=0 -DINPUT_METHOD -DSHOW_OPTION_KEY -DDO_COMDEX_HACK

MY_INCLUDES:= \
				-I$(BUILDHOME)/headers/add-ons/input_server \
				-I$(BUILDHOME)/$(SRC_DIR)/Analysis/AccessTable \
				-I$(BUILDHOME)/$(SRC_DIR)/Analysis/Analyze \
				-I$(BUILDHOME)/$(SRC_DIR)/Analysis/DicAccess \
				-I$(BUILDHOME)/$(SRC_DIR)/Analysis/DicCtrl \
				-I$(BUILDHOME)/$(SRC_DIR)/Analysis/DicData \
				-I$(BUILDHOME)/$(SRC_DIR)/Analysis/DoubleArray \
				-I$(BUILDHOME)/$(SRC_DIR)/Analysis/Grammar

SYSTEM_LIBS:= libroot.so libbe.so libergokanakan.a

PARENT_IMAGE:= $(INPUT_SERVER_OBJECT)

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/input_server/methods

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

include $(BUILDHOME)/src/addons/input_server/methods/japanese/Analysis/$(MAKEFILE_LEAFNAME)

