# CPUFeaturesTemplate.mk - CPU Features Template
# Copyright (c) 2000-2001; Be, Inc. All Rights Reserved.

# Parameters:
#   *TEMPLATE     := the template to call
#    CPU_FEATURES := space seperated list of CPU features
#                    ex: CPU_FEATURES := CMK CM C M3 M
#        Note: String compares are done with the CPU features so while CMK
#              will work, KMC will not.
#	GL_STYLE_PROCESSOR_DEFINES := a bit of a hack, but the
#					only way to do the task at hand.
#
# Note: All other input/output parameters come from the called
#   template.

TEMPLATE_IN := $(strip $(TEMPLATE))
CPU_FEATURES_IN := $(strip $(CPU_FEATURES))
GL_STYLE_PROCESSOR_DEFINES_IN := $(strip $(GL_STYLE_PROCESSOR_DEFINES))

ifeq ($(call MFLAG_IS_SET, USE_ANY_ACCEL_INSTRUCTIONS),)
include $(addprefix $(SDIR_TEMPLATES)/,$(TEMPLATE))
else
ifeq ($(CPU_FEATURES_IN),)
include $(addprefix $(SDIR_TEMPLATES)/,$(TEMPLATE))
else

TARGET_NAME_IN		:=	$(strip $(TARGET_NAME))
SRC_DIR_IN		:=	$(strip $(SRC_DIR))
SRCS_IN		:=	$(strip $(SRCS))
MY_CFLAGS_IN	:= $(strip $(MY_CFLAGS))
MY_INCLUDES_IN := $(strip $(MY_INCLUDES))
MY_OPTIMIZER_IN :=	$(strip $(MY_OPTIMIZER))
SYSTEM_LIBS_IN	:=	$(strip $(SYSTEM_LIBS))
PARENT_IMAGE_IN := $(strip $(PARENT_IMAGE))
HAS_MAIN_IN	:= $(strip $(HAS_MAIN))
LINK_DIRS_IN	:=	$(strip $(LINK_DIRS))
LINKS_IN		:=	$(strip $(LINKS))
MY_INSTALL_DIR_IN	:= 	$(strip $(MY_INSTALL_DIR))
SIGNATURE_IN	:=	$(strip $(SIGNATURE))
RESOURCES_IN	:=	$(strip $(RESOURCES))
ATTRIBUTES_IN	:=	$(strip $(ATTRIBUTES))
PROFILE_ME_IN := $(strip $(PROFILE_ME))
TYPE_IN		:=	$(strip $(TYPE))
OUR_CFLAGS_IN	:=	$(strip $(OUR_CFLAGS))
OUR_ASFLAGS_IN	:=	$(strip $(OUR_ASFLAGS))
OUR_INCLUDES_IN := $(strip $(OUR_INCLUDES))
OUR_OPTIMIZER_IN := $(strip $(OUR_OPTIMIZER))
OUR_CFLAGS_IN	:=	$(strip $(OUR_CFLAGS))
MORE_OBJS_IN	:=	$(strip $(MORE_OBJS))
MY_DEPS_IN := $(strip $(MY_DEPS))
MY_DEBUG_FLAGS_IN := $(strip $(MY_DEBUG_FLAGS))
OVERRIDE_CPU_FEATURES_DFLAGS_IN := $(strip $(OVERRIDE_CPU_FEATURES_DFLAGS))
OVERRIDE_CPU_ID_IN := $(strip $(OVERRIDE_CPU_ID))
OVERRIDE_M_CPU_IN := $(strip $(OVERRIDE_M_CPU))
OVERRIDE_M_ARCH_IN := $(strip $(OVERRIDE_M_ARCH))
MY_VERSION_IN := $(strip $(MY_VERSION))
MY_DFLAGS_IN := $(strip $(MY_DFLAGS))
MY_YACC_FLAGS_IN := $(strip $(MY_YACC_FLAGS))
MY_LEX_FLAGS_IN := $(strip $(MY_LEX_FLAGS))
MY_PREINCLUDE_IN := $(strip $(MY_PREINCLUDE))
MY_FIRST_INCLUDES_IN := $(strip $(MY_FIRST_INCLUDES))
WHOLE_ARCHIVES_IN := $(strip $(WHOLE_ARCHIVES))
UNDEFINED_SYMBOLS_IN := $(strip $(UNDEFINED_SYMBOLS))
RESOURCE_FORK_IN:=	 $(strip $(RESOURCE_FORK))
OVERRIDE_DO_SETVERSION_IN :=  $(strip $(OVERRIDE_DO_SETVERSION))
INSTALL_TARGET_IN := $(strip $(INSTALL_TARGET))
INSTALL_PREFIX_IN := $(strip $(INSTALL_PREFIX))

ifneq ($(filter katmai cmk CMK, $(CPU_FEATURES_IN)),)

TARGET_NAME		:=	$(TARGET_NAME_IN)
SRC_DIR		:=	$(SRC_DIR_IN)
SRCS		:=	$(SRCS_IN)
MY_CFLAGS	:= $(MY_CFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)
MY_OPTIMIZER :=	$(MY_OPTIMIZER_IN)
SYSTEM_LIBS	:=	$(SYSTEM_LIBS_IN)
PARENT_IMAGE := $(PARENT_IMAGE_IN)
HAS_MAIN	:= $(HAS_MAIN_IN)
LINK_DIRS	:=	$(LINK_DIRS_IN)
LINKS		:=	$(LINKS_IN)
MY_INSTALL_DIR	:= 	$(MY_INSTALL_DIR_IN)
SIGNATURE	:=	$(SIGNATURE_IN)
RESOURCES	:=	$(RESOURCES_IN)
ATTRIBUTES	:=	$(ATTRIBUTES_IN)
MY_DEPS := $(MY_DEPS_IN)
MY_DEBUG_FLAGS := $(MY_DEBUG_FLAGS_IN)
PROFILE_ME := $(PROFILE_ME_IN)
TYPE		:=	$(TYPE_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
OUR_ASFLAGS	:=	$(OUR_ASFLAGS_IN)
OUR_INCLUDES := $(OUR_INCLUDES_IN)
OUR_OPTIMIZER := $(OUR_OPTIMIZER_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
MORE_OBJS	:=	$(MORE_OBJS_IN)
MY_VERSION := $(MY_VERSION_IN)
MY_DFLAGS := $(MY_DFLAGS_IN)
MY_YACC_FLAGS := $(MY_YACC_FLAGS_IN)
MY_LEX_FLAGS := $(MY_LEX_FLAGS_IN)
MY_PREINCLUDE := $(MY_PREINCLUDE_IN)
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
WHOLE_ARCHIVES := $(WHOLE_ARCHIVES_IN)
UNDEFINED_SYMBOLS := $(UNDEFINED_SYMBOLS_IN)
RESOURCE_FORK := $(RESOURCE_FORK_IN)
OVERRIDE_DO_SETVERSION := $(OVERRIDE_DO_SETVERSION_IN)
INSTALL_TARGET := $(INSTALL_TARGET_IN)
INSTALL_PREFIX := $(INSTALL_PREFIX_IN)

OVERRIDE_CPU_FEATURES_DFLAGS := \
	-D_PROCESSOR_KNI_=1 \
	-D_PROCESSOR_MMX_=1 \
	-D_PROCESSOR_3DNOW_=0 \
	-D_PROCESSOR_CMOV_=1 \
#
ifneq ($(filter true 1, $(GL_STYLE_PROCESSOR_DEFINES)),)
OVERRIDE_CPU_FEATURES_DFLAGS += \
	-D__PROCESSOR_KATMAI__=1 \
	-D__PROCESSOR_MMX__=1 \
	-D__PROCESSOR_3DNOW__=0 \
	-D__PROCESSOR_P6__=1
endif
OVERRIDE_M_CPU := -mcpu=i686
OVERRIDE_M_ARCH := -march=i686
OVERRIDE_CPU_ID := CMK
include $(addprefix $(SDIR_TEMPLATES)/,$(TEMPLATE))
endif

ifneq ($(filter cm CM, $(CPU_FEATURES_IN)),)

TARGET_NAME		:=	$(TARGET_NAME_IN)
SRC_DIR		:=	$(SRC_DIR_IN)
SRCS		:=	$(SRCS_IN)
MY_CFLAGS	:= $(MY_CFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)
MY_OPTIMIZER :=	$(MY_OPTIMIZER_IN)
SYSTEM_LIBS	:=	$(SYSTEM_LIBS_IN)
PARENT_IMAGE := $(PARENT_IMAGE_IN)
HAS_MAIN	:= $(HAS_MAIN_IN)
LINK_DIRS	:=	$(LINK_DIRS_IN)
LINKS		:=	$(LINKS_IN)
MY_INSTALL_DIR	:= 	$(MY_INSTALL_DIR_IN)
SIGNATURE	:=	$(SIGNATURE_IN)
RESOURCES	:=	$(RESOURCES_IN)
ATTRIBUTES	:=	$(ATTRIBUTES_IN)
PROFILE_ME := $(PROFILE_ME_IN)
TYPE		:=	$(TYPE_IN)
MY_DEPS := $(MY_DEPS_IN)
MY_DEBUG_FLAGS := $(MY_DEBUG_FLAGS_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
OUR_ASFLAGS	:=	$(OUR_ASFLAGS_IN)
OUR_INCLUDES := $(OUR_INCLUDES_IN)
OUR_OPTIMIZER := $(OUR_OPTIMIZER_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
MORE_OBJS	:=	$(MORE_OBJS_IN)
MY_VERSION := $(MY_VERSION_IN)
MY_DFLAGS := $(MY_DFLAGS_IN)
MY_YACC_FLAGS := $(MY_YACC_FLAGS_IN)
MY_LEX_FLAGS := $(MY_LEX_FLAGS_IN)
MY_PREINCLUDE := $(MY_PREINCLUDE_IN)
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
WHOLE_ARCHIVES := $(WHOLE_ARCHIVES_IN)
UNDEFINED_SYMBOLS := $(UNDEFINED_SYMBOLS_IN)
RESOURCE_FORK := $(RESOURCE_FORK_IN)
OVERRIDE_DO_SETVERSION := $(OVERRIDE_DO_SETVERSION_IN)
INSTALL_TARGET := $(INSTALL_TARGET_IN)
INSTALL_PREFIX := $(INSTALL_PREFIX_IN)

OVERRIDE_CPU_FEATURES_DFLAGS := \
	-D_PROCESSOR_KNI_=0 \
	-D_PROCESSOR_MMX_=1 \
	-D_PROCESSOR_3DNOW_=0 \
	-D_PROCESSOR_CMOV_=1 \
#
ifneq ($(filter true 1, $(GL_STYLE_PROCESSOR_DEFINES)),)
OVERRIDE_CPU_FEATURES_DFLAGS += \
	-D__PROCESSOR_KATMAI__=0 \
	-D__PROCESSOR_MMX__=1 \
	-D__PROCESSOR_3DNOW__=0 \
	-D__PROCESSOR_P6__=1
endif
OVERRIDE_M_CPU := -mcpu=pentiumpro
OVERRIDE_M_ARCH := -march=pentiumpro
OVERRIDE_CPU_ID := CM
include $(addprefix $(SDIR_TEMPLATES)/,$(TEMPLATE))
endif

ifneq ($(filter c C, $(CPU_FEATURES_IN)),)

TARGET_NAME		:=	$(TARGET_NAME_IN)
SRC_DIR		:=	$(SRC_DIR_IN)
SRCS		:=	$(SRCS_IN)
MY_CFLAGS	:= $(MY_CFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)
MY_OPTIMIZER :=	$(MY_OPTIMIZER_IN)
SYSTEM_LIBS	:=	$(SYSTEM_LIBS_IN)
PARENT_IMAGE := $(PARENT_IMAGE_IN)
HAS_MAIN	:= $(HAS_MAIN_IN)
LINK_DIRS	:=	$(LINK_DIRS_IN)
LINKS		:=	$(LINKS_IN)
MY_INSTALL_DIR	:= 	$(MY_INSTALL_DIR_IN)
SIGNATURE	:=	$(SIGNATURE_IN)
RESOURCES	:=	$(RESOURCES_IN)
ATTRIBUTES	:=	$(ATTRIBUTES_IN)
PROFILE_ME := $(PROFILE_ME_IN)
TYPE		:=	$(TYPE_IN)
MY_DEPS := $(MY_DEPS_IN)
MY_DEBUG_FLAGS := $(MY_DEBUG_FLAGS_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
OUR_ASFLAGS	:=	$(OUR_ASFLAGS_IN)
OUR_INCLUDES := $(OUR_INCLUDES_IN)
OUR_OPTIMIZER := $(OUR_OPTIMIZER_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
MORE_OBJS	:=	$(MORE_OBJS_IN)
MY_VERSION := $(MY_VERSION_IN)
MY_DFLAGS := $(MY_DFLAGS_IN)
MY_YACC_FLAGS := $(MY_YACC_FLAGS_IN)
MY_LEX_FLAGS := $(MY_LEX_FLAGS_IN)
MY_PREINCLUDE := $(MY_PREINCLUDE_IN)
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
WHOLE_ARCHIVES := $(WHOLE_ARCHIVES_IN)
UNDEFINED_SYMBOLS := $(UNDEFINED_SYMBOLS_IN)
RESOURCE_FORK := $(RESOURCE_FORK_IN)
OVERRIDE_DO_SETVERSION := $(OVERRIDE_DO_SETVERSION_IN)
INSTALL_TARGET := $(INSTALL_TARGET_IN)
INSTALL_PREFIX := $(INSTALL_PREFIX_IN)

OVERRIDE_CPU_FEATURES_DFLAGS := \
	-D_PROCESSOR_KNI_=0 \
	-D_PROCESSOR_MMX_=0 \
	-D_PROCESSOR_3DNOW_=0 \
	-D_PROCESSOR_CMOV_=1 \
#
ifneq ($(filter true 1, $(GL_STYLE_PROCESSOR_DEFINES)),)
OVERRIDE_CPU_FEATURES_DFLAGS += \
	-D__PROCESSOR_KATMAI__=0 \
	-D__PROCESSOR_MMX__=0 \
	-D__PROCESSOR_3DNOW__=0 \
	-D__PROCESSOR_P6__=1
endif
OVERRIDE_M_CPU := -mcpu=pentiumpro
OVERRIDE_M_ARCH := -march=pentiumpro
OVERRIDE_CPU_ID := C
include $(addprefix $(SDIR_TEMPLATES)/,$(TEMPLATE))
endif

ifneq ($(filter 3dnow k6_3 k6_2 M3 m3, $(CPU_FEATURES_IN)),)

TARGET_NAME		:=	$(TARGET_NAME_IN)
SRC_DIR		:=	$(SRC_DIR_IN)
SRCS		:=	$(SRCS_IN)
MY_CFLAGS	:= $(MY_CFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)
MY_OPTIMIZER :=	$(MY_OPTIMIZER_IN)
SYSTEM_LIBS	:=	$(SYSTEM_LIBS_IN)
PARENT_IMAGE := $(PARENT_IMAGE_IN)
HAS_MAIN	:= $(HAS_MAIN_IN)
LINK_DIRS	:=	$(LINK_DIRS_IN)
LINKS		:=	$(LINKS_IN)
MY_INSTALL_DIR	:= 	$(MY_INSTALL_DIR_IN)
SIGNATURE	:=	$(SIGNATURE_IN)
RESOURCES	:=	$(RESOURCES_IN)
ATTRIBUTES	:=	$(ATTRIBUTES_IN)
PROFILE_ME := $(PROFILE_ME_IN)
TYPE		:=	$(TYPE_IN)
MY_DEPS := $(MY_DEPS_IN)
MY_DEBUG_FLAGS := $(MY_DEBUG_FLAGS_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
OUR_ASFLAGS	:=	$(OUR_ASFLAGS_IN)
OUR_INCLUDES := $(OUR_INCLUDES_IN)
OUR_OPTIMIZER := $(OUR_OPTIMIZER_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
MORE_OBJS	:=	$(MORE_OBJS_IN)
MY_VERSION := $(MY_VERSION_IN)
MY_DFLAGS := $(MY_DFLAGS_IN)
MY_YACC_FLAGS := $(MY_YACC_FLAGS_IN)
MY_LEX_FLAGS := $(MY_LEX_FLAGS_IN)
MY_PREINCLUDE := $(MY_PREINCLUDE_IN)
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
WHOLE_ARCHIVES := $(WHOLE_ARCHIVES_IN)
UNDEFINED_SYMBOLS := $(UNDEFINED_SYMBOLS_IN)
RESOURCE_FORK := $(RESOURCE_FORK_IN)
OVERRIDE_DO_SETVERSION := $(OVERRIDE_DO_SETVERSION_IN)
INSTALL_TARGET := $(INSTALL_TARGET_IN)
INSTALL_PREFIX := $(INSTALL_PREFIX_IN)

OVERRIDE_CPU_FEATURES_DFLAGS := \
	-D_PROCESSOR_KNI_=0 \
	-D_PROCESSOR_MMX_=1 \
	-D_PROCESSOR_3DNOW_=1 \
	-D_PROCESSOR_CMOV_=0 \
#
ifneq ($(filter true 1, $(GL_STYLE_PROCESSOR_DEFINES)),)
OVERRIDE_CPU_FEATURES_DFLAGS += \
	-D__PROCESSOR_KATMAI__=0 \
	-D__PROCESSOR_MMX__=1 \
	-D__PROCESSOR_3DNOW__=1 \
	-D__PROCESSOR_P6__=0
endif
OVERRIDE_M_CPU := -mcpu=pentiumpro
OVERRIDE_M_ARCH := -march=pentiumpro
OVERRIDE_CPU_ID := M3
include $(addprefix $(SDIR_TEMPLATES)/,$(TEMPLATE))
endif

ifneq ($(filter pentium pentium_mmx M m, $(CPU_FEATURES_IN)),)

TARGET_NAME		:=	$(TARGET_NAME_IN)
SRC_DIR		:=	$(SRC_DIR_IN)
SRCS		:=	$(SRCS_IN)
MY_CFLAGS	:= $(MY_CFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)
MY_OPTIMIZER :=	$(MY_OPTIMIZER_IN)
SYSTEM_LIBS	:=	$(SYSTEM_LIBS_IN)
PARENT_IMAGE := $(PARENT_IMAGE_IN)
HAS_MAIN	:= $(HAS_MAIN_IN)
LINK_DIRS	:=	$(LINK_DIRS_IN)
LINKS		:=	$(LINKS_IN)
MY_INSTALL_DIR	:= 	$(MY_INSTALL_DIR_IN)
SIGNATURE	:=	$(SIGNATURE_IN)
RESOURCES	:=	$(RESOURCES_IN)
ATTRIBUTES	:=	$(ATTRIBUTES_IN)
PROFILE_ME := $(PROFILE_ME_IN)
TYPE		:=	$(TYPE_IN)
MY_DEPS := $(MY_DEPS_IN)
MY_DEBUG_FLAGS := $(MY_DEBUG_FLAGS_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
OUR_ASFLAGS	:=	$(OUR_ASFLAGS_IN)
OUR_INCLUDES := $(OUR_INCLUDES_IN)
OUR_OPTIMIZER := $(OUR_OPTIMIZER_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
MORE_OBJS	:=	$(MORE_OBJS_IN)
MY_VERSION := $(MY_VERSION_IN)
MY_DFLAGS := $(MY_DFLAGS_IN)
MY_YACC_FLAGS := $(MY_YACC_FLAGS_IN)
MY_LEX_FLAGS := $(MY_LEX_FLAGS_IN)
MY_PREINCLUDE := $(MY_PREINCLUDE_IN)
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
WHOLE_ARCHIVES := $(WHOLE_ARCHIVES_IN)
UNDEFINED_SYMBOLS := $(UNDEFINED_SYMBOLS_IN)
RESOURCE_FORK := $(RESOURCE_FORK_IN)
OVERRIDE_DO_SETVERSION := $(OVERRIDE_DO_SETVERSION_IN)
INSTALL_TARGET := $(INSTALL_TARGET_IN)
INSTALL_PREFIX := $(INSTALL_PREFIX_IN)

OVERRIDE_CPU_FEATURES_DFLAGS := \
	-D_PROCESSOR_KNI_=0 \
	-D_PROCESSOR_MMX_=1 \
	-D_PROCESSOR_3DNOW_=0 \
	-D_PROCESSOR_CMOV_=0 \
#
ifneq ($(filter true 1, $(GL_STYLE_PROCESSOR_DEFINES)),)
OVERRIDE_CPU_FEATURES_DFLAGS += \
	-D__PROCESSOR_KATMAI__=0 \
	-D__PROCESSOR_MMX__=1 \
	-D__PROCESSOR_3DNOW__=0 \
	-D__PROCESSOR_P6__=0
endif
OVERRIDE_M_CPU := -mcpu=pentium
OVERRIDE_M_ARCH := -march=pentium
OVERRIDE_CPU_ID := M
include $(addprefix $(SDIR_TEMPLATES)/,$(TEMPLATE))
endif

TARGET_NAME		:=	$(TARGET_NAME_IN)
SRC_DIR		:=	$(SRC_DIR_IN)
SRCS		:=	$(SRCS_IN)
MY_CFLAGS	:= $(MY_CFLAGS_IN)
MY_INCLUDES := $(MY_INCLUDES_IN)
MY_OPTIMIZER :=	$(MY_OPTIMIZER_IN)
SYSTEM_LIBS	:=	$(SYSTEM_LIBS_IN)
PARENT_IMAGE := $(PARENT_IMAGE_IN)
HAS_MAIN	:= $(HAS_MAIN_IN)
LINK_DIRS	:=	$(LINK_DIRS_IN)
LINKS		:=	$(LINKS_IN)
MY_INSTALL_DIR	:= 	$(MY_INSTALL_DIR_IN)
SIGNATURE	:=	$(SIGNATURE_IN)
RESOURCES	:=	$(RESOURCES_IN)
ATTRIBUTES	:=	$(ATTRIBUTES_IN)
PROFILE_ME := $(PROFILE_ME_IN)
TYPE		:=	$(TYPE_IN)
MY_DEPS := $(MY_DEPS_IN)
MY_DEBUG_FLAGS := $(MY_DEBUG_FLAGS_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
OUR_ASFLAGS	:=	$(OUR_ASFLAGS_IN)
OUR_INCLUDES := $(OUR_INCLUDES_IN)
OUR_OPTIMIZER := $(OUR_OPTIMIZER_IN)
OUR_CFLAGS	:=	$(OUR_CFLAGS_IN)
MORE_OBJS	:=	$(MORE_OBJS_IN)
MY_VERSION := $(MY_VERSION_IN)
MY_DFLAGS := $(MY_DFLAGS_IN)
MY_YACC_FLAGS := $(MY_YACC_FLAGS_IN)
MY_LEX_FLAGS := $(MY_LEX_FLAGS_IN)
MY_PREINCLUDE := $(MY_PREINCLUDE_IN)
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
WHOLE_ARCHIVES := $(WHOLE_ARCHIVES_IN)
UNDEFINED_SYMBOLS := $(UNDEFINED_SYMBOLS_IN)
RESOURCE_FORK := $(RESOURCE_FORK_IN)
OVERRIDE_DO_SETVERSION := $(OVERRIDE_DO_SETVERSION_IN)
INSTALL_TARGET := $(INSTALL_TARGET_IN)
INSTALL_PREFIX := $(INSTALL_PREFIX_IN)

OVERRIDE_CPU_FEATURES_DFLAGS := \
	-D_PROCESSOR_KNI_=0 \
	-D_PROCESSOR_MMX_=0 \
	-D_PROCESSOR_3DNOW_=0 \
	-D_PROCESSOR_CMOV_=0 \
#
ifneq ($(filter true 1, $(GL_STYLE_PROCESSOR_DEFINES)),)
OVERRIDE_CPU_FEATURES_DFLAGS += \
	-D__PROCESSOR_KATMAI__=0 \
	-D__PROCESSOR_MMX__=0 \
	-D__PROCESSOR_3DNOW__=0 \
	-D__PROCESSOR_P6__=0
endif
OVERRIDE_M_CPU := -mcpu=pentium
OVERRIDE_M_ARCH := -march=pentium
OVERRIDE_CPU_ID := default
include $(addprefix $(SDIR_TEMPLATES)/,$(TEMPLATE))

endif
endif

TEMPLATE :=
CPU_FEATURES :=
GL_STYLE_PROCESSOR_DEFINES :=
