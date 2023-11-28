# Makefile

TARGET_NAME:=merlincontent.so

SRC_DIR:=src/addons/www_content/merlin

SRCS := MerlinContent.cpp \
		listview/MailListView.cpp \
		listview/MailListObserver.cpp \
		editor/CEditorView.cpp \
		editor/CTextView.cpp
		
MY_CFLAGS		:=

MY_INCLUDES		:=	-I$(BUILDHOME)/headers/controls \
					-I$(BUILDHOME)/headers/www \
					-I$(BUILDHOME)/headers/www/mail \
					-I$(LIBPNG_DIR) \
					-I$(BUILDHOME)/src/inc/support_p \
					-I$(BUILDHOME)/headers/experimental \
					-I$(BUILDHOME)/src/addons/www_content/merlin \
					-I$(BUILDHOME)/src/addons/www_content/merlin/editor \
					-I$(BUILDHOME)/src/addons/www_content/merlin/listview \



SYSTEM_LIBS		:= 	libpng.so \
					libroot.so \
					libbe.so \
					libcolumnlistview.so \
					libwww.so \
					libexperimental.a

PARENT_IMAGE	:=

HAS_MAIN		:=

MY_INSTALL_DIR	:=	$(INSTALL_ADD_ONS)/web

ATTRIBUTES		:=	-complex raw -t time 0 -t message	\
					"mime_types" "application/x-vnd.Be.merlin-listView"			\
					"mime_types" "application/x-vnd.Be.merlin-composeEditor"	\
					-end											\
					-name "be:content"								

LINK_DIRS		:=
SIGNATURE		:= 
RESOURCES		:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk
