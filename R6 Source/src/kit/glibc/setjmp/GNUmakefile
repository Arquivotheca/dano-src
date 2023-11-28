SRC_DIR:=src/kit/glibc/sysdeps/i386/elf
MY_FIRST_INCLUDES :=	\
	-I$(BUILDHOME)/src/kit/glibc/setjmp \
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
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/setjmp

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-DASSEMBLER \
		-DGAS_SYNTAX \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= setjmp.S

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/generic
MY_FIRST_INCLUDES :=	$(MY_FIRST_INCLUDES_IN)

# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/setjmp

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := $(MY_CFLAGS_IN)

SRCS:= sigjmp.c longjmp.c

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/i386
MY_FIRST_INCLUDES :=	$(MY_FIRST_INCLUDES_IN)

# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/setjmp

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := $(MY_CFLAGS_IN)

SRCS:= __longjmp.S

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
