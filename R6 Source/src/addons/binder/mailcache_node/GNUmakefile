
TARGET_NAME:=mailcache_node

SRC_DIR:=src/addons/binder/mailcache_node

SRCS := MailCacheNode.cpp \
		CacheContainer.cpp

MY_CFLAGS := -DUSE_OFFS=0

MY_INCLUDES:=	-I$(BUILDHOME)/headers/www \
				-I$(BUILDHOME)/headers/www/mail \
				-I$(BUILDHOME)/headers/xml

SYSTEM_LIBS:= libroot.so libbe.so libxml.so

PARENT_IMAGE:=

HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/binder

LINK_DIRS:=

SIGNATURE:=

RESOURCES:=

ATTRIBUTES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
