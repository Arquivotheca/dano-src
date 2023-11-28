SRC_DIR:=src/kit/glibc/wcsmbs-utf
MY_FIRST_INCLUDES :=	\
	-I$(BUILDHOME)/src/kit/glibc/wcsmbs-utf \
	-I$(BUILDHOME)/src/kit/glibc \
	-I$(BUILDHOME)/src/kit/glibc/include \
	-I$(BUILDHOME)/src/kit/glibc/libio \
	-I$(BUILDHOME)/src/kit/glibc/stdlib \
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

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	btowc.c \
	mblen.c \
	mbrlen.c \
	mbrtowc.c \
	mbsinit.c \
	mbsrtowcs.c \
	mbtowc.c \
	wcrtomb.c \
	wcscasecmp.c \
	wcscat.c \
	wcschr.c \
	wcscmp.c \
	wcscoll.c \
	wcscpy.c \
	wcscspn.c \
	wcslen.c \
	wcsncat.c \
	wcsncmp.c \
	wcsncpy.c \
	wcspbrk.c \
	wcsrchr.c \
	wcsrtombs.c \
	wcsspn.c \
	wcsstr.c \
	wcstod.c \
	wcstok.c \
	wcstol.c \
	wcstoq.c \
	wcstoul.c \
	wcstouq.c \
	wcsxfrm.c \
	wctob.c \
	wctomb.c \
	wmemchr.c \
	wmemcmp.c \
	wmemcpy.c \
	wmemmove.c \
	wmemset.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
