SRC_DIR:=src/kit/devel/i586

TARGET_NAME := libdevel.a

SRCS :=         \
	dis.c       \
	x86tables.c \
	cplus-dem.c

MY_CFLAGS := -D_BUILDING_devel=1

MY_INCLUDES := -I$(BUILDHOME)/headers/devel \
				-I$(BUILDHOME)/src/inc/os_p

MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)

SYSTEM_LIBS :=

LINKS:=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk
