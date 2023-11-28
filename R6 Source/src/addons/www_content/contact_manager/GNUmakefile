# Makefile

TARGET_NAME:=contactmanager.so

SRC_DIR:=src/addons/www_content/contact_manager

SRCS:=  ContactManagerContent.cpp \
	listview/TListView.cpp
		
MY_CFLAGS		:=

MY_INCLUDES		:=	-I$(BUILDHOME)/headers/controls \
					-I$(BUILDHOME)/headers/www \
					-I$(LIBPNG_DIR) \
					-I$(BUILDHOME)/src/inc/support_p \
					-I$(BUILDHOME)/src/addons/www_content/contact_manager \
					-I$(BUILDHOME)/src/addons/www_content/contact_manager/listview \

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
					"mime_types" "application/x-vnd.Be.ContactManager" \
					-end \
					-name "be:content"								

LINK_DIRS		:=
SIGNATURE		:= 
RESOURCES		:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk
