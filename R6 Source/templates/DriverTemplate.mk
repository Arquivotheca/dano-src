# DriverTemplate.mk -- Driver Template
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.
#
# Input Parameters: (they have to set before the template inclusion) 
#  *TARGET_NAME     := name of driver
#  *SRC_DIR         := offset of source directory
#  *SRCS            := space seperated list of sources
#   LINKS           := symlinks starting from the driver directory
#                      (dev/misc/$(TARGET_NAME) etc...)
#   MY_CFLAGS		:= driver-specific cflags
#   MY_INCLUDES		:= additional (private) headers you need, if any
#	MY_OPTIMIZER :=	the prefered optimizer setting
#	SYSTEM_LIBS     := the list of system libraries
#                      that your application links against.
#   PROFILE_ME  :=  If you should profile this add-on, set this to
#					'true' or '1'. -- False is assumed if empty, '0' or 'false'.
#   MY_DEPS:=       additional deps for all objects
#
# All the input parameters must exist. They should be set empty if they're
# unneeded. Items preceded with an asterisk (*) can not legally be empty.
#
# Important note:
#   Check included files for further details on everything.

# no recursive inclusion
ifdef INCLUDED_DRIVERTEMPLATE_INT
$(error a template cannot include itself (DriverTemplate))
endif
INCLUDED_DRIVERTEMPLATE_INT=1

## We only deal with our section of the parameters
## We let the included items deal with the rest.
TARGET_NAME_IN     := $(strip $(TARGET_NAME))
LINKS_IN           := $(strip $(LINKS))

#
# Most of this is handled by the KernelAddon.mk
#

INSTALL_TARGET := bin/$(TARGET_NAME_IN)
INSTALL_PREFIX := $(INSTALL_DRIVERS)

include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk
SRC_DIR:=$(SRC_DIR_IN)

# setup and include the symlink template

SOURCE := $(addprefix $(INSTALL_DRIVERS)/, bin/$(TARGET_NAME_IN))
LINK_PREFIX := $(INSTALL_DRIVERS)
# LINKS := $(LINKS)
LINK_RELATION := $(INSTALL_DRIVERS)
MY_DEPS := $(SDIR_TEMPLATES)/DriverTemplate.mk
include $(SDIR_TEMPLATES)/SymlinkTemplate.mk

# no recursive inclusion
INCLUDED_DRIVERTEMPLATE_INT=

TARGET_NAME     := 
LINKS           := 
