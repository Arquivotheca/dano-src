#
# net kit (compatibility) library
#

TARGET_NAME := libnet.so

SRC_DIR := src/kit/net

SRCS :=  \
	socket.c \
	netconfig.c \
	netdb_common.c
#

### Note: The whole '_BUILDING_blah stuff is unsupported in BOBS.
# ifeq ($(_BUILDING_RECOVERY_PARTITION),1)
# SRCS += net_no_thread_local.c
# else
SRCS += \
		allocator.cpp \
		netdebug.cpp \
#
# endif

ifneq ($(call MFLAG_IS_SET, USE_LIBBIND),)
  SRCS += netdb_libbind.c
else
  SRCS += \
		netdb.c \
		gethstnmad.c \
		res_comp.c \
		res_init.c \
		res_mkqry.c \
		res_query.c \
		res_send.c \
		res_debug.c \
		net_thread_local.c \
		init_term.c \
#
endif

MY_INCLUDES := \
	-Bnet_p -I$(BUILDHOME)/src/inc/net_p -Inet_p -I.

MY_CFLAGS := -DBSD=43

SYSTEM_LIBS := libroot.so libbind.so

MY_INSTALL_DIR := $(INSTALL_LIBS)

LINKS:=$(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk

