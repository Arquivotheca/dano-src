
TARGET_NAME		:=	auto_mounter_node

SRC_DIR			:=	src/addons/binder/auto_mounter_node

SRCS			:=											\
					AutoMounterNode.cpp						\
					DriveNode.cpp							\
					VolumeNode.cpp

MY_CFLAGS		:=	-DUSE_OFFS=0

MY_INCLUDES		:=											\
					-I$(BUILDHOME)/headers/www				\
					-I$(BUILDHOME)/headers/xml				\
					-I$(BUILDHOME)/src/inc/support_p		\
					-I$(BUILDHOME)/src/inc/www_p			\
					-I$(BUILDHOME)/src/inc/driver			\
					-I$(BUILDHOME)/src/inc/os_p				\
					-I.

SYSTEM_LIBS		:=	libroot.so								\
					libbe.so

PARENT_IMAGE	:=

HAS_MAIN		:=

MY_INSTALL_DIR	:=	$(INSTALL_ADD_ONS)/binder

LINK_DIRS		:=

SIGNATURE		:=

RESOURCES		:=

ATTRIBUTES		:=

MY_OPTIMIZER	:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk
