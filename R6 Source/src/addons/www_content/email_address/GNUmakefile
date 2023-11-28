# Makefile

TARGET_NAME := emailaddress.so

SRC_DIR := src/addons/www_content/email_address

SRCS := EmailAddressContent.cpp AutoCompleteControl.cpp
		
MY_CFLAGS :=

MY_INCLUDES :=   -I$(BUILDHOME)/headers/controls \
                 -I$(BUILDHOME)/headers/www \
                 -I$(BUILDHOME)/headers/xml \
                 -I$(BUILDHOME)/src/inc/support_p \
                 -I$(BUILDHOME)/src/kit/interface \
                 -I$(BUILDHOME)/src/addons/www_content/email_address



SYSTEM_LIBS :=         libroot.so \
					libbe.so \
					libwww.so

PARENT_IMAGE :=

HAS_MAIN :=

MY_INSTALL_DIR := $(INSTALL_ADD_ONS)/web

ATTRIBUTES :=  -complex raw -t time 0 -t message	\
					"mime_types" "application/x-vnd.Be.EmailAddress" \
					-end \
					-name "be:content"	

LINK_DIRS		:=
SIGNATURE		:= 
RESOURCES		:=

include $(SDIR_TEMPLATES)/AddonTemplate.mk

