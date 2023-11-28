SRC_DIR:=src/kit/glibc/sysdeps/beos
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
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/stdio-common

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-D_IO_MTSAFE_IO \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	ctermid.c \
	cuserid.c \
	rename.c \
	siglist.c \
	tempname.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/posix
SRC_DIR:=src/kit/glibc/sysdeps/beos
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
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/stdio-common

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-D_IO_MTSAFE_IO \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	lockfile.c \

MY_TRASH_ID := $(word 1, $(SRCS))

MY_OPTIMIZER := -finline

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/posix
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/stdio-common
SRCS:= \
	remove.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/stdio-common
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
SRCS:= \
	_itoa.c \
	asprintf.c \
	fprintf.c \
	fscanf.c \
	getline.c \
	itoa-digits.c \
	perror.c \
	printf.c \
	printf_fp.c \
	scanf.c \
	snprintf.c \
	sprintf.c \
	sscanf.c \
	tempnam.c \
	tmpfile.c \
	tmpnam.c \
	vfprintf.c \
	vfscanf.c \
	vprintf.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

