TARGET_NAME:=imap.so

SRC_DIR:=src/addons/email/imap

SRCS := \
	MailIMAPAnswerList.cpp \
	MailIMAPClient.cpp \
	MailIMAPClientData.cpp \
	MailIMAPTools.cpp

MY_CFLAGS :=  -DMAIL_CLIENT_IMAP_DEBUG

MY_INCLUDES:= -I$(BUILDHOME)/src/inc/beia_mail_p -I$(BUILDHOME)/src/inc/beia_mail_p/md5

SYSTEM_LIBS:=libroot.so libbe.so libbeia_mail.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/email

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
