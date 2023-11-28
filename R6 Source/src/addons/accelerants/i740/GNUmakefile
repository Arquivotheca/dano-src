TARGET_NAME:= i740.accelerant

SRC_DIR:=src/addons/accelerants/i740

SRCS		:= i740.c thunk.c accel.c bena4.c

MY_CFLAGS := -DDEBUG=1 -DCOMPILING_ACCELERANT

MY_INCLUDES:= \
	-I$(BUILDHOME)/src/inc \
#

SYSTEM_LIBS:=libroot.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/accelerants

LINK_DIRS:=

SIGNATURE	:= "application/x-vnd.Be-i740.accelerant"

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

