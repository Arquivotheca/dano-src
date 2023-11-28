
TARGET_NAME:=deres

SRC_DIR:=src/commands/restools/$(TARGET_NAME)

SRCS := $(TARGET_NAME).cpp

MY_INCLUDES := -I. \
				-I$(BUILDHOME)/headers/reseditor \
				-I$(BUILDHOME)/headers/resparser \

RESOURCES	:= ${TARGET_NAME}.rsrc

SYSTEM_LIBS:= libroot.so libbe.so libexperimental.a \
	libreseditor.so libresparser.so

MY_INSTALL_DIR:=$(INSTALL_BIN)

include $(SDIR_TEMPLATES)/AppTemplate.mk

