
TARGET_NAME:= translate
SRC_DIR := src/commands/be/$(TARGET_NAME)

# RESOURCES := $(TARGET_NAME).rsrc

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := \
	${TARGET_NAME}.cpp

MY_CFLAGS:= 

MY_INCLUDES += -I$(BUILDHOME)/src/inc/app_p

SYSTEM_LIBS:= libbe.so libroot.so libtranslation.so

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk
