
TARGET_NAME		:= picasso_test_views
SRC_DIR			:= src/apps/picasso_test_views
MY_INSTALL_DIR	:= $(INSTALL_APPS)

SRCS			:= \
					main.cpp \
#

SYSTEM_LIBS		:=									\
				libroot.so							\
				libbe2.so							\
#

MY_FIRST_INCLUDES		:=							\
				-I$(BUILDHOME)/headers/
#

LINK_DIRS := $(INSTALL_USER_APPS)

MY_OPTIMIZER	:=
MY_DFLAGS		:= -g -O0

MORE_OBJS:=$(BELIBRARIES)/binder_addon_glue.o

include $(SDIR_TEMPLATES)/AppTemplate.mk

