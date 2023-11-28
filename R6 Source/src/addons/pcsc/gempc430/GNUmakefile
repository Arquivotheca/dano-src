# gemcore addon

TARGET_NAME := gempc430

SRC_DIR := src/addons/pcsc/gempc430

SRCS			:=	\
					LLDInterface.c		\
					T0cases.c			\
					ReaderTransport.c	\
					isoIO.c				\
					ReaderManagement.c	\
					RMInterface.c		\
					usbserial.c

MY_CFLAGS :=

MY_INCLUDES		:=	-I$(BUILDHOME)/headers/pcsc

MY_INSTALL_DIR = $(INSTALL_ADD_ONS)/pcsc

SYSTEM_LIBS:= libroot.so

PARENT_IMAGE :=
HAS_MAIN :=
LINK_DIRS :=
SIGNATURE :=
ATTRIBUTES := 
RESOURCES :=


include $(SDIR_TEMPLATES)/AddonTemplate.mk

