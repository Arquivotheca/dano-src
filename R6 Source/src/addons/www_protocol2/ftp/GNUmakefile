# Makefile

TARGET_NAME:=ftpprotocol.so

SRC_DIR:=src/addons/www_protocol2/ftp

SRCS := FtpProtocol.cpp \
		FtpConnection.cpp \
		FtpConnectionPool.cpp \
		ftpparse.c \
#

MY_CFLAGS := $(F_NO_EXCEPTIONS)

ifneq ($(call MFLAG_IS_SET, LOG_FTP),)
        MY_DFLAGS += -DENABLE_FTP_TRACE=1
endif

MY_INCLUDES:=	-I$(BUILDHOME)/headers/support2 \
				-I$(BUILDHOME)/headers/www2 \
				-I$(BUILDHOME)/headers/xml2 \
#

SYSTEM_LIBS:= libroot.so libbe2.so libwww2.so libbnetapi.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"schemes" "ftp"						\
			-t bool "be:protocol" true				\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
