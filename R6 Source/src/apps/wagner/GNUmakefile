TARGET_NAME		:=	Wagner

MY_INSTALL_DIR	:=	$(INSTALL_APPS)

SRC_DIR			:=	src/apps/wagner

SRCS			:=									\
				Wotan.cpp							\
				WebBinderNode.cpp					\
				ViewTree.cpp						\
				BrowserWindow.cpp

SYSTEM_LIBS		:=									\
				libroot.so							\
				libbe.so							\
				libwww.so

RESOURCE_FORK	:= $(TARGET_NAME).rsrc

MY_INCLUDES		:=									\
				-I$(BUILDHOME)/headers/www			\
				-I$(BUILDHOME)/headers/xml			\
				-I$(BUILDHOME)/src/inc/driver		\
				-I$(BUILDHOME)/src/inc/www_p		\
				-I$(BUILDHOME)/src/inc/support_p	\
				-I$(BUILDHOME)/src/inc/print_p		\
				-I$(BUILDHOME)/src/kit/interface		\
				-I$(BUILDHOME)/src/inc/os_p

MY_CFLAGS		:=	$(F_NO_EXCEPTIONS)

ifneq ($(call MFLAG_IS_SET,LIBWWW_CONTAINS_MAIL),)
MY_CFLAGS		+=	-DINCLUDE_MAIL_CACHE
endif

ifneq ($(call MFLAG_IS_SET, PRINTING_FOR_DESKTOP),)
MY_CFLAGS		+= -DPRINTING_FOR_DESKTOP=1
else
MY_CFLAGS		+= -DPRINTING_FOR_DESKTOP=0
endif


LINK_DIRS		:=	$(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)

MY_OPTIMIZER	:=

include $(SDIR_TEMPLATES)/AppTemplate.mk
