# Makefile

TARGET_NAME:=file_browser2.so

SRC_DIR:=src/addons/www_content/file_browser2

SRCS			:=											\
					FileBrowserContent.cpp					\
					FileBrowserView.cpp						\
					FileListView.cpp						\
					Editor.cpp

MY_CFLAGS		:=

MY_INCLUDES		:=											\
					-I$(BUILDHOME)/src/inc/os_p				\
				  	-I$(BUILDHOME)/headers/www				\
				 	-I$(BUILDHOME)/headers/controls			\
				 	-I$(BUILDHOME)/src/inc/www_p			\
				 	-I$(BUILDHOME)/src/addons/www_protocol/ftp

SYSTEM_LIBS		:=											\
					libbe.so								\
					libroot.so								\
					libwww.so								\
					libcolumnlistview.so

PARENT_IMAGE	:=

HAS_MAIN		:=

MY_INSTALL_DIR	:=	$(INSTALL_ADD_ONS)/web

ATTRIBUTES		:=	-complex raw -t time 0 -t message		\
			"mime_types" "application/x-vnd.Be.FileBrowser"	\
			-end											\
			-name "be:content"

LINK_DIRS		:=
SIGNATURE		:= 
RESOURCES		:=

MY_OPTIMIZER:=
include $(SDIR_TEMPLATES)/AddonTemplate.mk
