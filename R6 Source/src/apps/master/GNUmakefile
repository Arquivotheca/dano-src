TARGET_NAME		:=	master

MY_INSTALL_DIR	:=	$(INSTALL_APPS)

SRC_DIR			:=	src/apps/master

SRCS			:=									\
				master.cpp							\
				md5.c								\
				versioncache.cpp					\
				catfile.cpp							\
				db-postgres.cpp						\
				util.cpp							\
				config.cpp							\
				sqlutil.cpp

SYSTEM_LIBS		:=									\
				libroot.so							\
				libbe.so							\
				libsocket.so						\
				libbind.so							

MORE_OBJS := lib/libpq++.a lib/libpq.a

#RESOURCE_FORK	:= $(TARGET_NAME).rsrc

MY_INCLUDES		:=									\
				-I./include							\
				-I$(BUILDHOME)/headers/www			\
				-I$(BUILDHOME)/src/inc/driver		\
				-I$(BUILDHOME)/src/inc/www_p		\
				-I$(BUILDHOME)/src/inc/support_p	\
				-I$(BUILDHOME)/src/inc/os_p

MY_CFLAGS		:=	$(F_NO_EXCEPTIONS)

LINK_DIRS		:=	$(INSTALL_USER_APPSMENU) $(INSTALL_USER_APPS)

MY_OPTIMIZER	:=	-O0 -g

include $(SDIR_TEMPLATES)/HostAppTemplate.mk
