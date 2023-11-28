# Makefile

TARGET_NAME:=jpeg.so

SRC_DIR:=src/addons/www_content/jpeg

SRCS := JPEG.cpp JPEGContent.cpp JPEGDraw.cpp JPEGDCT.cpp JPEGScan.cpp

MY_CFLAGS :=

MY_INCLUDES:= -I$(BUILDHOME)/headers/www

SYSTEM_LIBS:= libroot.so libbe.so libwww.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

ATTRIBUTES := -complex raw -t time 0 -t message		\
			"mime_types" "image/jpeg"				\
			"extensions" "jpeg"						\
			"extensions" "jpg"						\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
