
TARGET_NAME:= makebootable
SRC_DIR := src/commands/be/$(TARGET_NAME)

RESOURCES :=

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := $(TARGET_NAME).cpp

MY_DFLAGS:= -Dxor=XOR -Dor=OR -Dand=AND -Dnot=NOT

MY_INCLUDES +=	-I$(BUILDHOME)/src/inc/os_p \
				-I$(BUILDHOME)/src/inc \
				-I$(BUILDHOME)/src/nukernel/inc \
				-I$(BUILDHOME)/src/fs/bfs

SYSTEM_LIBS:= libbe.so libroot.so

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk
