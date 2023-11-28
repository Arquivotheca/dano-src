
TARGET_NAME:= roster
SRC_DIR := src/commands/be/$(TARGET_NAME)

MY_INSTALL_DIR:=$(INSTALL_BIN)

SRCS := \
	$(TARGET_NAME).cpp

MY_DFLAGS:= -DDEBUG=1

MY_INCLUDES	:= \
#

SYSTEM_LIBS:= libbe.so libroot.so \
#

LINK_DIRS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk
