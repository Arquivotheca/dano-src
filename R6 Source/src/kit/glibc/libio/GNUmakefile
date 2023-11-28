SRC_DIR:=src/kit/glibc/sysdeps/posix
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
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/libio

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-D_IO_MTSAFE_IO \
		-DIO_DEBUG \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	libc_fatal.c

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/libio
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
SRCS:= \
	clearerr.c \
	clearerr_u.c \
	fcloseall.c \
	feof.c \
	feof_u.c \
	ferror.c \
	ferror_u.c \
	fgetc.c \
	filedoalloc.c \
	fileno.c \
	fileops.c \
	fputc.c \
	fputc_u.c \
	freopen.c \
	fseek.c \
	fseeko.c \
	ftello.c \
	genops.c \
	getc.c \
	getc_u.c \
	getchar.c \
	getchar_u.c \
	iofclose.c \
	iofdopen.c \
	iofflush.c \
	iofflush_u.c \
	iofgetpos.c \
	iofgets.c \
	iofgets_u.c \
	iofopen.c \
	iofputs.c \
	iofread.c \
	iofread_u.c \
	iofsetpos.c \
	ioftell.c \
	iofwrite.c \
	iofwrite_u.c \
	iogetdelim.c \
	iogetline.c \
	iogets.c \
	iopadn.c \
	iopopen.c \
	ioputs.c \
	ioseekoff.c \
	ioseekpos.c \
	iosetbuffer.c \
	iosetvbuf.c \
	iosprintf.c \
	ioungetc.c \
	iovdprintf.c \
	iovsprintf.c \
	iovsscanf.c \
	pclose.c \
	peekc.c \
	putc.c \
	putc_u.c \
	putchar.c \
	putchar_u.c \
	rewind.c \
	setbuf.c \
	setlinebuf.c \
	stdfiles.c \
	stdio.c \
	strops.c \
	vasprintf.c \
	vscanf.c \
	vsnprintf.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

