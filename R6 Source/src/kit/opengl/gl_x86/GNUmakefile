# Makefile using CPUFeaturesTemplate.mk and SimpleSharedLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libGL2.so

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/kit/opengl/gl_x86

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := \
	Loader.cpp \
	DirectGLWindow.cpp \
	DirectGLWindow_Cruft.cpp \
	asm.c \
	assembler.c \
	accum.c \
	attrib.c \
	bitmap.c \
	clip.c \
	context.c \
	depth.c \
	dither.c \
	eval.c \
	feedback.c \
	finish.c \
	fog.c \
	g_api.c \
	g_lcomp.c \
	g_lexec.c \
	g_ltab.c \
	g_tab.c \
	get.c \
	global.c \
	hashMap.c \
	hints.c \
	idMgr.c \
	image.c \
	lighting.c \
	line.c \
	lineclip.c \
	linedraw.c \
	list.c \
	listcomp.c \
	matrix.c \
	memservices.c \
	pick.c \
	pixel.c \
	point.c \
	polyclip.c \
	polygon.c \
	prim.c \
	prim_processor_generation.c \
	rasBuffers.c \
	rasScanline.c \
	rasTriangle.c \
	raster.c \
	rect.c \
	ScanlineRaster.c \
	select.c \
	stencil.c \
	stub.c \
	surface.c \
	texture2.c \
	validate.c \
	vapi.c \
	vapi_opts.c \
	vapi_generation.c \
	vector.c \
	vertarr.c \
	vertex.c \
	xform.c \
	converter_MMX.asm \
	prim_processor_parts_intel.asm \
	rasTriangle_intel.asm \
	util_intel.asm \
	vector_intel.asm \
	master.asm \
	gllc_stubs_intel.asm \
	scanline_parts_intel.asm \
	scanline_x86_default.asm \
	scanline_x86_default_Generation.c \
	stub_intel.asm

MY_DFLAGS := \
	-DLIBRARYBUILD=1 \
	-D_BUILDING_gl=1 \
	-DNO_STATIC=1 \
#

# MY_DFLAGS - Template User provided -dflags.
ifeq ($(TARGET_CPU),i586)
MY_DFLAGS += \
	-D__PROCESSOR_USE_C__=0 \
	-D__PROCESSOR_P5__=1 \
#
else
MY_DFLAGS += \
	-D__PROCESSOR_USE_C__=1 \
	-D__PROCESSOR_P5__=0 \
#
endif

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := \
			-I. \
			-I$(BUILDHOME)/src/inc/app_p \
			-I$(BUILDHOME)/src/inc/os_p \
			-I$(BUILDHOME)/src/inc/app_server_p \
			-I$(BUILDHOME)/src/inc/support_p \
			-I$(BUILDHOME)/src/inc/interface_p \
			-I$(BUILDHOME)/src/inc/input_server_p \
			-I$(BUILDHOME)/headers/add-ons/graphics \
			$(call FULL_OBJ_DIR, $(SRC_DIR)) \
#

MY_WARNINGS := $(WARN_NONE)

MY_OPTIMIZER := -ffixed-ebx -no-fpic


# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so libbe.so libgame.so libbitflinger.so

# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. 
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
MY_INSTALL_DIR := $(INSTALL_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.so
LINKS := 

# PROFILE_ME - If you should profile this shared lib, set this to
#              'true' or '1'. -- False is assumed if empty, '0' or 'false'.
# PROFILE_ME := 1

# MORE_OBJS - additional objects to link to the target
#             these may be absolute paths and are not cleaned up
# MORE_OBJS :=

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

# RESOURCES - the file(s) containing the resources you want to add to your target.
#             The suffix(es) should be '.rsrc' or '.r'.
# RESOURCES :=

MY_DEPS := $(call FULL_OBJ_DIR, $(SRC_DIR))/state.h

TEMPLATE := SimpleSharedLibTemplate.mk

CPU_FEATURES := CMK CM C M3 M

# A bit of a hack, but the only way to do the task at hand.
GL_STYLE_PROCESSOR_DEFINES := true

include $(SDIR_TEMPLATES)/CPUFeaturesTemplate.mk

# The link must be handled seperately, since $(TARGET_NAME) is messed with
# (changed to default.so).
SOURCE	:= /boot/beos/system/lib/libGL2.so

SRC_DIR := src/kit/opengl/gl_x86

LINK_PREFIX		:=
LINK_RELATION	:=
LINKS			:=$(INSTALL_DEV_LIBS)/libGL2.so
MY_DEPS			:=$(MAKEFILE_LEAFNAME)

include $(SDIR_TEMPLATES)/SymlinkTemplate.mk


SOURCE := state.hcs
SRC_DIR:=src/kit/opengl/gl_x86
TARGET:=$(call FULL_OBJ_DIR, $(SRC_DIR))/$(SOURCE)
include $(SDIR_TEMPLATES)/HostDataFileTemplate.mk

# Makefile using GenericGeneratedTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the installed created file (can not be empty)
TARGET_NAME := state.h

# MY_RULE - the name of the rule to use -- Remember the rule won't have
#  it's variables expanded when it's defined. Rules should be defined in
#  Macros.mk and end in _RULE. (can not be empty)
MY_RULE := OPENGL_STATE_H_RULE

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
# NOTE: Either SRC_DIR or MY_INSTALL_DIR need to be defined. The target
#  will be in the OBJ_DIR if MY_INSTALL_DIR is undefined.
SRC_DIR:=$(SRC_DIR_IN)

# MY_DEPS - the list of fully qualified items to depend upon
MY_DEPS := $(call FULL_OBJ_DIR, $(SRC_DIR_IN))/state.hcs $(OPENGL_DHC)

include $(SDIR_TEMPLATES)/GenericGeneratedTemplate.mk

