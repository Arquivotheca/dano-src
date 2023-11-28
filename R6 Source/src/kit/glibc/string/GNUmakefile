SRC_DIR:=src/kit/glibc/sysdeps/generic
MY_FIRST_INCLUDES :=	\
	-I$(BUILDHOME)/src/kit/glibc/stdlib \
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

# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/string

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-DASSEMBLER \
		-DGAS_SYNTAX \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	bcopy.c \
	memmove.c \
	strcasecmp.c \
	strcmp.c \
	strncat.c \
	strncmp.c \
	strncpy.c \
	strstr.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/i386
MY_FIRST_INCLUDES :=	$(MY_FIRST_INCLUDES_IN)

# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/string

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := $(MY_CFLAGS_IN)

SRCS:= \
	ffs.c \
	memchr.S \
	memcmp.S \
	strcspn.S \
	strpbrk.S \
	strrchr.S \
	strspn.S \
	strtok.S \
	strtok_r.S \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/i386/i486
MY_FIRST_INCLUDES :=	$(MY_FIRST_INCLUDES_IN)

# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/string

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := $(MY_CFLAGS_IN)

SRCS:= \
	strcat.S

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/i386/i586
MY_FIRST_INCLUDES :=	$(MY_FIRST_INCLUDES_IN)

# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/string

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := $(MY_CFLAGS_IN)

SRCS:= \
	bzero.S \
	stpcpy.S \
	strchr.S \
	strcpy.S \
	strlen.S \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/string
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
SRCS:= \
	strcoll.c \
	strdup.c \
	strnlen.c \
	strxfrm.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
