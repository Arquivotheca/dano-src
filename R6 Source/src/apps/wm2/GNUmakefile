
TARGET_NAME		:= wm2
SRC_DIR			:= src/apps/wm2
MY_INSTALL_DIR	:= $(INSTALL_APPS)

SRCS			:= \
					main.cpp \
					wm.cpp \
					WMRootView.cpp \
#

SYSTEM_LIBS		:=									\
				libroot.so							\
				libbe2.so							\
#

MY_FIRST_INCLUDES		:=							\
				-I$(BUILDHOME)/headers/				\
				-I$(BUILDHOME)/src/inc/				\
#

LINK_DIRS := $(INSTALL_USER_APPS)

MY_OPTIMIZER	:=
MY_DFLAGS		:= -g -O0

MORE_OBJS:=$(BELIBRARIES)/binder_addon_glue.o

include $(SDIR_TEMPLATES)/AppTemplate.mk

