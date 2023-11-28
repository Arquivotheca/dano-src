
TARGET_NAME		:= stella
SRC_DIR			:= src/apps/stella
MY_INSTALL_DIR	:= $(INSTALL_APPS)

SRCS			:= \
					mainBeOS.cpp \
					SoundBeOS.cpp \
					TIASoundBeOS.cpp \
#

SYSTEM_LIBS		:=									\
				libroot.so							\
				libbe2.so							\
				libmedia.so							\
				libstdc++.r4.so						\
#

MY_FIRST_INCLUDES		:=								\
				-I$(BUILDHOME)/headers/
#

MORE_OBJS :=	\
				$(BUILDHOME)/$(SRC_DIR)/libstella.a \
				$(BELIBRARIES)/binder_addon_glue.o	\

MY_CFLAGS := $(F_INLINE) $(F_NO_PIC) $(F_NO_EXCEPTIONS) -O3
MY_DFLAGS := -DBE -DENABLE_DRAWING=1 -DBSPF_UNIX
LINK_DIRS := $(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)

MY_OPTIMIZER	:=

include $(SDIR_TEMPLATES)/AppTemplate.mk

# install a few games

FILES			:= frogger.bin jrpacman.bin pacman.bin
MY_INSTALL_DIR	:= $(INSTALL_BASE)/home/config/etc/stella
SRC_DIR			:= src/apps/stella

include $(SDIR_TEMPLATES)/DataTemplate.mk
