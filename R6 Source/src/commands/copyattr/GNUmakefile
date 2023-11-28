# copyattr
TARGET_NAME:=copyattr
SYSTEM_LIBS:= libroot.so
MY_INCLUDES:=
SRCS:=copyattr.c
MY_INSTALL_DIR:=$(INSTALL_BIN)
SRC_DIR:=src/commands/copyattr
MY_CFLAGS:=

include $(SDIR_TEMPLATES)/AppTemplate.mk

# copyattr
TARGET_NAME:=$(TARGET_NAME_IN)
SRC_DIR:=$(SRC_DIR_IN)
SYSTEM_LIBS:= $(SYSTEM_LIBS_IN)
MY_INCLUDES:= $(MY_INCLUDES_IN)
SRCS:= $(SRCS_IN)
MY_CFLAGS:=$(MY_CFLAGS_IN)

include $(SDIR_TEMPLATES)/HostAppTemplate.mk
