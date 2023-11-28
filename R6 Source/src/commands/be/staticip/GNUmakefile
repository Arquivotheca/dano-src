# staticip/dynamicip makefile

# --- staticip ---

TARGET_NAME:= staticip
SRC_DIR := src/commands/be/staticip
SRCS:= staticip.c

MY_INSTALL_DIR := $(INSTALL_BIN)

MY_CFLAGS:=
MY_INCLUDES	:=

SYSTEM_LIBS:= libroot.so

LINK_DIRS:=
SIGNATURE:=
RESOURCES:=


include $(SDIR_TEMPLATES)/AppTemplate.mk

# --- dynamicip ---

TARGET_NAME:= dynamicip
SRC_DIR := src/commands/be/staticip
SRCS:= dynamicip.c

MY_INSTALL_DIR := $(INSTALL_BIN)

MY_CFLAGS:=
MY_INCLUDES	:=

SYSTEM_LIBS:= libroot.so

LINK_DIRS:= 
SIGNATURE:=
RESOURCES:=


include $(SDIR_TEMPLATES)/AppTemplate.mk

# --- crufty temporary scripts
FILES 			:= 	kill_net_server kill_wagner spawn_net_server\
#


SRC_DIR			:= src/commands/be/staticip

MY_INSTALL_DIR	:=	$(INSTALL_BIN)

PERMISSIONS     := $(PERMS_USER_APP)

include $(SDIR_TEMPLATES)/DataTemplate.mk
