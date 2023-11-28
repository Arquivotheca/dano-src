# InterfaceSpecTemplate.mk -- template for an interface spec
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.
#
# Note: This is for .spec files living in ${BUILDHOME}/config/spec/interface.
#
# Input Parameters: (they have to set before the template inclusion) 
#  *SPEC_NAME :=	the name of the device
#	PARENT_SPEC :=	the name of the device this was based upon
#   ALL_ELF :=		files required for any package with this device
#   ALL_PLAIN :=	files required for any package with this device
#   ALL_DIR :=		files required for any package with this device
#   ALL_LINK :=		files required for any package with this device
#	ALL_MFLAGS :=	mflags required for any package with this device
#	ALL_DFLAGS :=	dflags required for any package with this device
#	ALL_MVARS :=	mvars required for any package with this device
#   NOT_ALL_ELF :=	files not installed for any package with this device
#				(that otherwise would be installed)
#   NOT_ALL_PLAIN :=files not installed for any package with this device
#				(that otherwise would be installed)
#   NOT_ALL_DIR :=	files not installed for any package with this device
#				(that otherwise would be installed)
#   NOT_ALL_LINK :=	files not installed for any package with this device
#				(that otherwise would be installed)
#	NOT_ALL_MFLAGS := mflags not installed for any package with this device
#				(that otherwise would be installed)
#	NOT_ALL_DFLAGS := dflags not installed for any package with this device
#				(that otherwise would be installed)

#
# Output Parameters: (they become valid after the template inclusion, and are
# invalidated by any other template inclusion)
#
#	$(SPEC_NAME)-based variables
#	All input variables are cleared.
#   The only input variables carried over to *_IN variables
#   are SPEC_NAME and PARENT_SPEC
#
# Todo:
#
#	There are still sections that can't be customized.
#	They should be added as needed.
#
# Bugs:
#
#   Because of the sheer number of input variables, most don't have _IN equilivents.
#
# Notes:
#	The PARENT_SPEC *must* be defined before any of the
#	input variables to this are set.
#
#   The heirarchy of packages should be irrelivant to this.
#	The INTERFACE is included unless it's set to NA.
#
############################################################

# This file is actually nothing more than a giant list
# of definitions.

# Strip the variables as we plop them in their output locations
SPEC_NAME_IN := $(strip $(SPEC_NAME))
PARENT_SPEC_IN :=	$(strip $(PARENT_SPEC))


$(SPEC_NAME_IN)_FILES_ELF_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_FILES_ELF_SPEC),) \
	$(ALL_ELF) \
#

$(SPEC_NAME_IN)_FILES_PLAIN_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_FILES_PLAIN_SPEC),) \
	$(ALL_PLAIN) \
#

$(SPEC_NAME_IN)_FILES_LINK_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_FILES_LINK_SPEC),) \
	$(ALL_LINK) \
#

$(SPEC_NAME_IN)_FILES_GRAPHICS_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_FILES_GRAPHICS_SPEC),) \
	$(ALL_GRAPHICS) \
#

$(SPEC_NAME_IN)_FILES_LBX_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_FILES_LBX_SPEC),) \
	$(ALL_LBX) \
#

$(SPEC_NAME_IN)_FILES_DIR_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_FILES_DIR_SPEC)),) \
	$(ALL_DIR) \
#

$(SPEC_NAME_IN)_MFLAGS_SPEC := \
	$(if $(PARENT_SPEC_IN), $(filter-out $($($(PARENT_SPEC_NAME)_CPU_TYPE_SPEC)_MPAK),$($(PARENT_SPEC_IN)_MFLAGS_SPEC)),) \
	$($($(SPEC_NAME)_CPU_TYPE_SPEC)_MPAK) \
	$(ALL_MFLAGS) \
#

$(SPEC_NAME_IN)_DFLAGS_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_DFLAGS_SPEC),) \
	$(ALL_DFLAGS) \
#

$(SPEC_NAME_IN)_MVARS_SPEC := \
	$(call MVAR_REPLACE, $(ALL_MVARS), $($(PARENT_SPEC_IN)_MVARS_SPEC)) \
#

$(SPEC_NAME_IN)_REMOVE_FILES_ELF_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_REMOVE_FILES_ELF_SPEC),) \
	$(NOT_ALL_ELF) \
#

$(SPEC_NAME_IN)_REMOVE_FILES_PLAIN_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_REMOVE_FILES_PLAIN_SPEC),) \
	$(NOT_ALL_PLAIN) \
#

$(SPEC_NAME_IN)_REMOVE_FILES_LINK_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_REMOVE_FILES_LINK_SPEC),) \
	$(NOT_ALL_LINK) \
#

$(SPEC_NAME_IN)_FILES_REMOVE_GRAPHICS_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_REMOVE_FILES_GRAPHICS_SPEC),) \
	$(NOT_ALL_GRAPHICS) \
#

$(SPEC_NAME_IN)_FILES_REMOVE_LBX_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_REMOVE_FILES_LBX_SPEC),) \
	$(NOT_ALL_LBX) \
#

$(SPEC_NAME_IN)_REMOVE_FILES_DIR_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_REMOVE_FILES_DIR_SPEC)),) \
	$(NOT_ALL_DIR) \
#

$(SPEC_NAME_IN)_REMOVE_MFLAGS_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_REMOVE_MFLAGS_SPEC),) \
	$(NOT_ALL_MFLAGS) \
#

$(SPEC_NAME_IN)_REMOVE_DFLAGS_SPEC := \
	$(if $(PARENT_SPEC_IN), $($(PARENT_SPEC_IN)_REMOVE_DFLAGS_SPEC),) \
	$(NOT_ALL_DFLAGS) \
#

# Clear all input variables

SPEC_NAME:=	
PARENT_SPEC:=	
ALL_ELF :=		
ALL_PLAIN :=	
ALL_DIR :=		
ALL_LINK :=		
ALL_MFLAGS :=	
ALL_DFLAGS :=	
ALL_MVARS :=
ALL_GRAPHICS :=
ALL_LBX :=
NOT_ALL_ELF :=	
NOT_ALL_PLAIN :=
NOT_ALL_DIR :=	
NOT_ALL_LINK :=	
NOT_ALL_MFLAGS := 
NOT_ALL_DFLAGS := 
NOT_ALL_GRAPHICS :=
NOT_ALL_LBX :=

