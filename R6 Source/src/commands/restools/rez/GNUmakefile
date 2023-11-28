# Copyright 1996, 1997, 1998
#         Hekkelman Programmatuur B.V.  All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#    
#     This product includes software developed by Hekkelman Programmatuur B.V.
# 
# 4. The name of Hekkelman Programmatuur B.V. may not be used to endorse or
#    promote products derived from this software without specific prior
#    written permission.
# 
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

TARGET_NAME:=rez

SRC_DIR:=src/commands/restools/$(TARGET_NAME)

SRCS :=  \
	rez_parser.y \
	rez_scanner.l \
	RState.cpp \
	SymbolTable.cpp \
	rez.cpp \
	REval.cpp \
	RElem.cpp \

SYSTEM_LIBS:= libroot.so libbe.so libstdc++.r4.so

# MY_DEPS := $(MAKEFILE_LEAFNAME)

# RESOURCES:= $(TARGET_NAME).rsrc

MY_INCLUDES := -I. \
	-I$(call FULL_OBJ_DIR, $(SRC_DIR)) \

MY_YACC_FLAGS := $(BISON_CPP_OUTPUT_FLAG)

MY_LEX_FLAGS := $(FLEX_CPP_OUTPUT_FLAG) -i

MY_INSTALL_DIR:=$(INSTALL_BIN)

include $(SDIR_TEMPLATES)/AppTemplate.mk

DEPEND_ON_ME := rez_parser.h
SRC_DIR := $(SRC_DIR_IN)
SRCS := $(SRCS_IN)
include $(SDIR_TEMPLATES)/NoCircleDepTemplate.mk

# Copyright 1996, 1997, 1998
#         Hekkelman Programmatuur B.V.  All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#    
#     This product includes software developed by Hekkelman Programmatuur B.V.
# 
# 4. The name of Hekkelman Programmatuur B.V. may not be used to endorse or
#    promote products derived from this software without specific prior
#    written permission.
# 
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

TARGET_NAME:=rez

SRC_DIR:=src/commands/restools/$(TARGET_NAME)

SRCS :=  \
	rez_parser.y \
	rez_scanner.l \
	RState.cpp \
	SymbolTable.cpp \
	rez.cpp \
	REval.cpp \
	RElem.cpp \

SYSTEM_LIBS:= libroot.so libbe.so libstdc++.r4.so

# MY_DEPS := $(MAKEFILE_LEAFNAME)

# RESOURCES:= $(TARGET_NAME).rsrc

MY_INCLUDES := -I. \
			   -I$(call FULL_OBJ_DIR, $(SRC_DIR), $(HOST_MY_PREFIX)) \

MY_YACC_FLAGS := $(BISON_CPP_OUTPUT_FLAG)

MY_LEX_FLAGS := $(FLEX_CPP_OUTPUT_FLAG) -i

include $(SDIR_TEMPLATES)/HostAppTemplate.mk

DEPEND_ON_ME := rez_parser.h
SRC_DIR := $(SRC_DIR_IN)
SRCS := $(SRCS_IN)
MY_OBJ_DIR := $(call FULL_OBJ_DIR, $(SRC_DIR), $(HOST_MY_PREFIX))
include $(SDIR_TEMPLATES)/NoCircleDepTemplate.mk

