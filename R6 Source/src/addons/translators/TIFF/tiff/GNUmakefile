
# Makefile using GenericGeneratedTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the installed created file (can not be empty)
TARGET_NAME := g3states.h

# MY_RULE - the name of the rule to use -- Remember the rule won't have
#  it's variables expanded when it's defined. Rules should be defined in
#  Macros.mk and end in _RULE. (can not be empty)
MY_RULE := TIFF_G3STATES_RULE

# MY_DEPS - the list of fully qualified items to depend upon
MY_DEPS := $(TIFF_MKG3STATES)

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
# NOTE: Either SRC_DIR or MY_INSTALL_DIR need to be defined. The target
#  will be in the OBJ_DIR if MY_INSTALL_DIR is undefined.
SRC_DIR:=src/addons/translators/TIFF/tiff

# MY_INSTALL_DIR - the absolute destination directory -- should be an
#  INSTALL_ constant. (defaults to OBJ_DIR) If it defaults to the OBJ_DIR
#  it's treated as a build item and not an installed item. (It's attached
#  to the build vs. install rules.)
# NOTE: Either SRC_DIR or MY_INSTALL_DIR need to be defined. The target
#  will be in the OBJ_DIR if MY_INSTALL_DIR is undefined.
MY_INSTALL_DIR :=

include $(SDIR_TEMPLATES)/GenericGeneratedTemplate.mk


# Makefile using StaticLibTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the file. (can not be empty)
TARGET_NAME:=libtiff.a

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=src/addons/translators/TIFF/tiff

MY_DEPS += $(TARGET)

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS := $(patsubst $(OBJ)/%.o,%.cpp, \
	$(OBJ)/tif_fax3.o \
	$(OBJ)/tif_fax4.o \
	$(OBJ)/tif_aux.o \
	$(OBJ)/tif_ccittrle.o \
	$(OBJ)/tif_close.o \
	$(OBJ)/tif_compress.o \
	$(OBJ)/tif_dir.o \
	$(OBJ)/tif_dirinfo.o \
	$(OBJ)/tif_dirread.o \
	$(OBJ)/tif_dirwrite.o \
	$(OBJ)/tif_dumpmode.o \
	$(OBJ)/tif_error.o \
	$(OBJ)/tif_getimage.o \
	$(OBJ)/tif_jpeg.o \
	$(OBJ)/tif_flush.o \
	$(OBJ)/tif_lzw.o \
	$(OBJ)/tif_next.o \
	$(OBJ)/tif_open.o \
	$(OBJ)/tif_packbits.o \
	$(OBJ)/tif_print.o \
	$(OBJ)/tif_read.o \
	$(OBJ)/tif_strip.o \
	$(OBJ)/tif_swab.o \
	$(OBJ)/tif_thunder.o \
	$(OBJ)/tif_tile.o \
	$(OBJ)/tif_be.o \
	$(OBJ)/tif_version.o \
	$(OBJ)/tif_warning.o \
	$(OBJ)/tif_write.o \
	$(OBJ)/TIFFReader.o \
)

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
	-I$(call FULL_OBJ_DIR, $(SRC_DIR))
#

# MY_OPTIMIZER - the prefered optimization level


# MY_INSTALL_DIR - the full path of the directory you want
#				   your app to be installed in. 
# You should use of the globally define INSTALL_* variables defined in
#   InstallTree.mk. 
# MY_INSTALL_DIR := $(INSTALL_DEV_LIBS)


# LINKS	 - symlinks in the install tree. Should be based on the INSTALL
#          variables. 
# Example: LINKS := $(INSTALL_DEV_LIBS)/libFOO-5000.a
# LINKS :=

# MY_DEBUG_FLAGS - additional flags to set when DEBUG_BUILD is used.
# MY_DEBUG_FLAGS :=

include $(SDIR_TEMPLATES)/StaticLibTemplate.mk

# Makefile using HostAppTemplate.mk
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# TARGET_NAME - the name of the Executable. (can not be empty)
TARGET_NAME:=tiff-mkg3states

# SRC_DIR - the path of the root of your application source tree. (Preferably
#           relative to your source tree's base.) (can not be empty)
SRC_DIR:=$(SRC_DIR_IN)

# SRCS - the list of source files relative to $(SRC_DIR). (can not be empty)
SRCS:=mkg3states.cpp

# MY_CFLAGS - Template User provided flags.
# MY_CFLAGS :=

# MY_INCLUDES - Additional headers (as might be appended to CFLAGS)
MY_INCLUDES := -I$(call FULL_SRC_DIR, $(SRC_DIR)) \
#

# SYSTEM_LIBS - the list of system libraries (ie located in $(BELIBRARIES))
#               that your application links against.
SYSTEM_LIBS:= libroot.so

# RESOURCES - the file(s) containing the resources you want to add to your target.
#             The suffix(es) should be '.rsrc' or '.r'.
# RESOURCES :=

include $(SDIR_TEMPLATES)/HostAppTemplate.mk

