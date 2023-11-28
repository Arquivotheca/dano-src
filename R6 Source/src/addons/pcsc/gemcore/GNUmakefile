# gemcore addon

TARGET_NAME := gemcore

SRC_DIR := src/addons/pcsc/gemcore

SRCS			:=	\
					apdubuil.c		\
					apduspli.c		\
					or32gem.c		\
					or3confi.c		\
					or3gbpcl.c		\
					or3gbpco.c		\
					or3gbpop.c		\
					or3utils.c		\
					t0case1.c		\
					t0case2.c		\
					t0case3.c		\
					t0case4.c  		\
					gtgbp.c			\
					gttimout.c		\
					gserial.c		\
					IFD_Handler.c	\
					or3icc.c

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

