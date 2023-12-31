# Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU Library General Public
# License along with the GNU C Library; see the file COPYING.LIB.  If not,
# write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

# Makefile for the math library.

subdir		:= math

# Installed header files.
headers		:= math.h bits/mathcalls.h bits/mathinline.h bits/huge_val.h \
		   bits/nan.h fpu_control.h complex.h bits/cmathcalls.h \
		   fenv.h bits/fenv.h bits/mathdef.h tgmath.h

# Internal header files.
distribute	:= math_private.h machine/asm.h machine/endian.h

# FPU support code.
aux		:= fpu_control setfpucw

# Build the -lm library.

include ../Makeconfig

extra-libs	:= libm
extra-libs-others = $(extra-libs)
ifeq (yes,$(build-shared))
#others: $(objpfx)libm.so
#$(objpfx)libm.so: $(objpfx)libm_pic.a
#	$(build-shlib)
endif

libm-support = k_standard s_lib_version s_matherr s_signgam		\
	       fclrexcpt fgetexcptflg fraiseexcpt fsetexcptflg		\
	       ftestexcept fegetround fesetround fegetenv feholdexcpt	\
	       fesetenv feupdateenv t_exp
libm-calls = e_acos e_acosh e_asin e_atan2 e_atanh e_cosh e_exp e_fmod	\
	     e_hypot e_j0 e_j1 e_jn e_lgamma_r e_log e_log10 e_pow	\
	     e_rem_pio2 e_remainder e_scalb e_sinh e_sqrt e_gamma_r	\
	     k_cos k_rem_pio2 k_sin k_tan s_asinh s_atan s_cbrt		\
	     s_ceil s_cos s_erf s_expm1 s_fabs	\
	     s_floor s_ilogb s_log1p s_logb 		\
	     s_nextafter s_nextafterx s_rint s_scalbln	\
	     s_significand s_sin s_tan s_tanh w_acos w_acosh w_asin	\
	     w_atan2 w_atanh w_cosh w_drem w_exp w_exp2 w_exp10 w_fmod	\
	     w_gamma w_hypot w_j0 w_j1 w_jn w_lgamma w_lgamma_r		\
	     w_log w_log10 w_pow w_remainder w_scalb w_sinh w_sqrt	\
	     s_fpclassify s_fmax s_fmin s_fdim s_nan s_trunc	\
	     s_remquo s_log2 s_exp2 s_round s_nearbyint s_sincos	\
	     conj cimag creal cabs carg s_cexp s_csinh s_ccosh s_clog	\
	     s_catan s_casin s_ccos s_csin s_ctan s_ctanh s_cacos	\
	     s_casinh s_cacosh s_catanh s_csqrt s_cpow s_cproj s_clog10 \
	     s_fma s_lrint s_llrint s_lround s_llround e_exp10
libm-routines = $(libm-support) $(libm-calls) \
		$(patsubst %_rf,%f_r,$(libm-calls:=f))	\
		$(long-m-$(long-double-fcts))
long-m-routines = $(patsubst %_rl,%l_r,$(libm-calls:=l))
#long-m-yes = $(long-m-routines)
distribute += $(long-m-yes:=.c)

# These functions are in libc instead of libm because __printf_fp
# calls them, so any program using printf will need them linked in,
# and we don't want to have to link every program with -lm.
calls = s_isinf s_isnan s_finite s_copysign s_modf s_scalbn s_frexp s_ldexp \
	s_signbit
routines = $(calls) $(calls:=f) $(calls:=l)
#long-c-yes = $(calls:=l)
distribute += $(long-c-yes:=.c)

# Rules for the test suite.
tests = test-float test-double $(test-longdouble-$(long-double-fcts)) \
	test-ifloat test-idouble test-matherr test-fenv
tests-static = atest-exp atest-sincos atest-exp2
# We do the `long double' tests only if this data type is available and
# distinct from `double'.
test-longdouble-yes = test-ldouble test-ildoubl

CFLAGS-test-float.c = -fno-inline -ffloat-store
CFLAGS-test-double.c = -fno-inline -ffloat-store
CFLAGS-test-ldouble.c = -fno-inline -ffloat-store
CFLAGS-test-ifloat.c = -U__LIBC_INTERNAL_MATH_INLINES
CFLAGS-test-idouble.c = -U__LIBC_INTERNAL_MATH_INLINES
CFLAGS-test-ildoubl.c = -U__LIBC_INTERNAL_MATH_INLINES
LDLIBS-test-ifloat = math/libm
LDLIBS-test-idouble = math/libm
LDLIBS-test-ildoubl = math/libm
LDLIBS-test-float = math/libm
LDLIBS-test-double = math/libm
LDLIBS-test-ldouble = math/libm
LDLIBS-test-matherr = math/libm
LDLIBS-test-reduce = math/libm
LDLIBS-atest-exp = math/libm
LDLIBS-atest-sincos = math/libm
LDLIBS-atest-exp2 = math/libm

distribute += libm-test.c


# The -lieee module sets the _LIB_VERSION_ switch to IEEE mode
# for error handling in the -lm functions.
install-lib += libieee.a
non-lib.a += libieee.a
extra-objs += libieee.a ieee-math.o
distribute += ieee-math.c

include ../Rules


# This file defines the default _LIB_VERSION variable that controls
# the error return conventions for the math functions.
CPPFLAGS-s_lib_version.c := -D_POSIX_MODE

# We don't want the fdlibm code to use the inline math functions,
# only the fdlibm code.
CPPFLAGS += -D__NO_MATH_INLINES -D__LIBC_INTERNAL_MATH_INLINES

# Using omit-long-double-fcts is only a temporary hack.
ifneq ($(omit-long-double-fcts),yes)
ifneq ($(long-double-fcts),yes)
# The `double' and `long double' types are the same on this machine.
# We won't compile the `long double' code at all.  Tell the `double' code
# to define aliases for the `FUNCl' names.  To avoid type conflicts in
# defining those aliases, tell <math.h> to declare the `FUNCl' names with
# `double' instead of `long double'.
CPPFLAGS += -DNO_LONG_DOUBLE -D_Mlong_double_=double
endif
endif

# The fdlibm code generates a lot of these warnings but is otherwise clean.
override CFLAGS += -Wno-uninitialized -Wno-write-strings

# The -lieee library is actually an object file.
# The module just defines the _LIB_VERSION_ variable.
# It's not a library to make sure it is linked in instead of s_lib_version.o.
$(objpfx)libieee.a: $(objpfx)ieee-math.o
	rm -f $@
	$(LN_S) $< $@

ifeq ($(build-shared),yes)
$(addprefix $(objpfx),$(tests)): $(objpfx)libm.so$(libm.so-version)
else
$(addprefix $(objpfx),$(tests)): $(objpfx)libm.a
endif

# Depend on libc.so so a DT_NEEDED is generated in the shared objects.
# This ensures they will load libc.so for needed symbols if loaded by
# a statically-linked program that hasn't already loaded it.
#$(objpfx)libm.so: $(common-objpfx)libc.so
