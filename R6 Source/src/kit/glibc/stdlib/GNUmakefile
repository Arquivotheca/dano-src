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
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/stdlib

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= alloca.c

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/generic
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/stdlib
SRCS:= \
	abort.c \
	cmp.c \
	div.c \
	divmod_1.c \
	divrem.c \
	inlines.c \
	ldiv.c \
	lldiv.c \
	mod_1.c \
	udiv_qrnnd.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/generic
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/stdlib
SRCS:= \
	mul.c \
	mul_n.c \

MY_TRASH_ID := $(word 1, $(SRCS))

MY_OPTIMIZER := -finline

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/i386/i586
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN) -DHAVE_INITFINI -DASSEMBLER -DGAS_SYNTAX
# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/stdlib
SRCS:= \
	add_n.S \
	addmul_1.S \
	lshift.S \
	mul_1.S \
	rshift.S \
	sub_n.S \
	submul_1.S \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/ieee754
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/stdlib
SRCS:= \
	dbl2mpn.c \
	mpn2dbl.c \
	mpn2flt.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/wordsize-32
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
# Is there a better way to do this?
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/stdlib
SRCS:= \
	strtoimax.c \
	strtoumax.c \
	wcstoimax.c \
	wcstoumax.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/stdlib
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
SRCS:= \
	abs.c \
	atof.c \
	atoi.c \
	atol.c \
	atoll.c \
	drand48-iter.c \
	drand48.c \
	drand48_r.c \
	erand48.c \
	erand48_r.c \
	exit.c \
	fpioconst.c \
	jrand48.c \
	jrand48_r.c \
	labs.c \
	lcong48.c \
	lcong48_r.c \
	llabs.c \
	lrand48.c \
	lrand48_r.c \
	mbstowcs.c \
	mrand48.c \
	mrand48_r.c \
	nrand48.c \
	nrand48_r.c \
	rand.c \
	rand_r.c \
	random_r.c \
	seed48.c \
	seed48_r.c \
	srand48.c \
	srand48_r.c \
	strtol.c \
	strtoll.c \
	strtoul.c \
	strtoull.c \
	wcstombs.c \

# In gcc3 and later __clz_tab is part of the gcc runtime in libgcc.
# For gcc2, we need the table from glibc.
ifeq ($(call MVAR_TO_TARGET_NAME, COMPILER_VERSION, gcc2),gcc2) 
SRCS+=mp_clz_tab.c
endif
$(call MVAR_CHECK_FINITE_OPTIONS, COMPILER_VERSION, gcc2 gcc3, gcc2)

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/stdlib
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
SRCS:= \
	strtod.c \

MY_TRASH_ID := $(word 1, $(SRCS))

MY_OPTIMIZER := -finline

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/stdlib
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN)
SRCS:= \
	atexit.c \
	random.c \

MY_TRASH_ID := $(word 1, $(SRCS))

MY_OPTIMIZER := -finline

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/stdlib
MY_FIRST_INCLUDES := $(MY_FIRST_INCLUDES_IN)
MY_CFLAGS := $(MY_CFLAGS_IN) -fexceptions
SRCS:= \
	bsearch.c \
	msort.c \
	qsort.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
