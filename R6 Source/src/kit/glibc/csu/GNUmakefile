SRC_DIR:=src/kit/glibc/sysdeps/beos
MY_FIRST_INCLUDES :=	\
	-I$(BUILDHOME)/src/kit/glibc/csu \
	-I$(BUILDHOME)/src/kit/glibc \
	-I$(BUILDHOME)/src/kit/glibc/include \
	-I$(BUILDHOME)/src/kit/glibc/libio \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/i386/elf \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/beos \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/posix \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/i386/i586 \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/i386/i486 \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/i386/fpu \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/libm-i387 \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/i386 \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/wordsize-32 \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/ieee754 \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/libm-ieee754 \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/generic/elf \
	-I$(BUILDHOME)/src/kit/glibc/sysdeps/generic \

# (fnf) Is there a better way to do this?
# (steven) There are other ways to do it, but it'll work. You just need to set
#   a MY_TRASH_ID so the clean rules don't over-lap.
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/csu

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-DHAVE_INITFINI \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	init-first.c \
	libc-start.c \
	sysdep.c \

# Add the first source to the trash id to make it unique when combined with
# the OBJ_DIR.
MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/csu
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
SRCS:= \
	abi-note.S \
	gmon-start.c \
	set-init.c \
	version.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/i386/elf
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN) -DHAVE_INITFINI -DASSEMBLER -DGAS_SYNTAX
# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/csu
SRCS:= start.S

# Add the first source to the trash id to make it unique when combined with
# the OBJ_DIR.
MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

TARGET_NAME:=crt1.o
SRC_DIR:=src/kit/glibc/csu
# MORE_OBJS also supports objects checked in to the tree, so the symantic is that
# the filenames are relative to SRC_DIR. If you do specify the object directory as
# the source directory, you will want to explicitly set the OBJ_DIR to the same value,
# so that the $(call FULL_OBJ_DIR) logic doesn't get confused and make the OBJ_DIR
# something utterly bizzar like $(MY_PREFIX)/_root/$(SRC_DIR)).
# -- In case you didn't catch that, the SRC_DIR may be absolute, and being absolute
#    would be required if it is set to an OBJ_DIR. (steven 2001-06-29)
MORE_OBJS:= $(addprefix $(call FULL_OBJ_DIR, $(SRC_DIR))/, \
	start.o abi-note.o \
)

include $(SDIR_TEMPLATES)/ObjectCombiningTemplate.mk

TARGET_NAME:=gcrt1.o
SRC_DIR:=src/kit/glibc/csu
MORE_OBJS:= $(addprefix $(call FULL_OBJ_DIR, $(SRC_DIR))/, \
	crt1.o gmon-start.o \
)

include $(SDIR_TEMPLATES)/ObjectCombiningTemplate.mk
