TARGET_NAME:=contact_local.so

SRC_DIR:=src/addons/email/contact_local

SRCS := \
	ContactLOCALClient.cpp \
	ContactLOCALClientData.cpp

MY_CFLAGS :=  -DCONTACT_CLIENT_LOCAL_DEBUG

MY_INCLUDES:= -I$(BUILDHOME)/src/inc/beia_mail_p

SYSTEM_LIBS:=libroot.so libbe.so libbeia_mail.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/email

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
