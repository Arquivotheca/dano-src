# psloop
TARGET_NAME:=psloop
SRC_DIR:=src/commands/$(TARGET_NAME)

SRCS:=$(TARGET_NAME).c

MY_CFLAGS:=
MY_INCLUDES:= -I$(call FULL_SRC_DIR, src/commands/ps)

SYSTEM_LIBS:= libroot.so
RESOURCE_FORK:=

MY_INSTALL_DIR:=$(INSTALL_BIN)
LINK_DIRS:=


include $(SDIR_TEMPLATES)/AppTemplate.mk
