SRC_DIR:=src/kit/mail

TARGET_NAME := libmail.so

SRCS :=  \
	E-mail.cpp \
	SettingsFile.cpp

MY_DFLAGS :=  -D_BUILDING_$(strip $(notdir $(SRC_DIR)))=1 -DBSD=43 

MY_INCLUDES	:= -I$(BUILDHOME)/src/inc/mail_p	\
			   -I$(BUILDHOME)/src/inc/net_p		\
#

MY_INSTALL_DIR := $(INSTALL_LIBS)

SYSTEM_LIBS := libroot.so libbe.so libtextencoding.so libsocket.so

LINKS := $(INSTALL_DEV_LIBS)/$(TARGET_NAME)

include $(SDIR_TEMPLATES)/SimpleSharedLibTemplate.mk


