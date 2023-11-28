TARGET_NAME:=httpprotocol.so

SRC_DIR:=src/addons/www_protocol/http

SRCS := HTTP.cpp addon.cpp HTTPUtil.cpp

MY_CFLAGS := $(F_NO_EXCEPTIONS) -Wall -Wno-long-long -Wno-multichar

#
# For debugging, make with 'make FORCE_MFLAGS=LOG_HTTP'
# all of the request/response headers will be printed to stdout.
#
ifneq ($(call MFLAG_IS_SET, LOG_HTTP),)
        MY_DFLAGS += -DENABLE_HTTP_TRACE=1
endif

MY_INCLUDES:= -I$(BUILDHOME)/headers/www -I$(BUILDHOME)/src/inc/www_p

SYSTEM_LIBS:= libroot.so libbe.so libwww.so libsocket.so

PARENT_IMAGE:=
HAS_MAIN:=

MY_INSTALL_DIR:=$(INSTALL_ADD_ONS)/web

# Install the identifier attributes for this add-on.
# BE AWARE: Any changes to these attributes should also
# be made to the factory class.
# Also note that we set a modification time of 0, so
# the attributes will never be overwritten from the
# executable, even if it is later changed.
ATTRIBUTES := -complex raw -t time 0 -t message		\
			"schemes" "http"						\
			"schemes" "https"						\
			-t bool "be:protocol" true				\
			-end									\
			-name "be:content"

LINK_DIRS:=
SIGNATURE:= 
RESOURCES:=


include $(SDIR_TEMPLATES)/AddonTemplate.mk
