SRC_DIR:=src/kit/glibc/sysdeps/libm-ieee754
MY_FIRST_INCLUDES :=	\
	-I$(BUILDHOME)/src/kit/glibc/math \
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
OBJ_DIR := $(MY_PREFIX)/src/kit/glibc/math

# MY_CFLAGS - Template User provided flags.
# Note that only s_lib_version.o needs -D_POSIX_MODE
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-D__NO_MATH_INLINES \
		-D__LIBC_INTERNAL_MATH_INLINES \
		-D_POSIX_MODE \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	e_cosh.c \
	e_coshf.c \
	e_gamma_r.c \
	e_gammaf_r.c \
	e_hypot.c \
	e_hypotf.c \
	e_j0.c \
	e_j0f.c \
	e_j1.c \
	e_j1f.c \
	e_jn.c \
	e_jnf.c \
	e_lgamma_r.c \
	e_lgammaf_r.c \
	e_sinh.c \
	e_sinhf.c \
	k_cos.c \
	k_cosf.c \
	k_sin.c \
	k_sinf.c \
	k_standard.c \
	k_tan.c \
	k_tanf.c \
	s_cacos.c \
	s_cacosf.c \
	s_cacosh.c \
	s_cacoshf.c \
	s_casin.c \
	s_casinf.c \
	s_casinh.c \
	s_casinhf.c \
	s_catan.c \
	s_catanf.c \
	s_catanh.c \
	s_catanhf.c \
	s_ccos.c \
	s_ccosf.c \
	s_ccosh.c \
	s_ccoshf.c \
	s_clog.c \
	s_clog10.c \
	s_clog10f.c \
	s_clogf.c \
	s_cpow.c \
	s_cpowf.c \
	s_cproj.c \
	s_cprojf.c \
	s_csin.c \
	s_csinf.c \
	s_csinh.c \
	s_csinhf.c \
	s_csqrt.c \
	s_csqrtf.c \
	s_ctan.c \
	s_ctanf.c \
	s_ctanh.c \
	s_ctanhf.c \
	s_erf.c \
	s_erff.c \
	s_fabs.c \
	s_fabsf.c \
	s_fpclassify.c \
	s_fpclassifyf.c \
	s_isinf.c \
	s_isinff.c \
	s_isnan.c \
	s_isnanf.c \
	s_ldexp.c \
	s_ldexpf.c \
	s_ldexpl.c \
	s_lib_version.c \
	s_llround.c \
	s_llroundf.c \
	s_lround.c \
	s_lroundf.c \
	s_matherr.c \
	s_modf.c \
	s_modff.c \
	s_modfl.c \
	s_nan.c \
	s_nanf.c \
	s_nextafter.c \
	s_nextafterf.c \
	s_nextafterx.c \
	s_nextafterxf.c \
	s_round.c \
	s_roundf.c \
	s_signbit.c \
	s_signbitf.c \
	s_signbitl.c \
	s_signgam.c \
	s_tanh.c \
	s_tanhf.c \
	w_acos.c \
	w_acosf.c \
	w_acosh.c \
	w_acoshf.c \
	w_asin.c \
	w_asinf.c \
	w_atan2.c \
	w_atan2f.c \
	w_atanh.c \
	w_atanhf.c \
	w_cosh.c \
	w_coshf.c \
	w_drem.c \
	w_dremf.c \
	w_exp.c \
	w_exp10.c \
	w_exp10f.c \
	w_exp2.c \
	w_exp2f.c \
	w_expf.c \
	w_fmod.c \
	w_fmodf.c \
	w_gamma.c \
	w_gammaf.c \
	w_hypot.c \
	w_hypotf.c \
	w_j0.c \
	w_j0f.c \
	w_j1.c \
	w_j1f.c \
	w_jn.c \
	w_jnf.c \
	w_lgamma.c \
	w_lgamma_r.c \
	w_lgammaf.c \
	w_lgammaf_r.c \
	w_log.c \
	w_log10.c \
	w_log10f.c \
	w_logf.c \
	w_pow.c \
	w_powf.c \
	w_remainder.c \
	w_remainderf.c \
	w_scalb.c \
	w_scalbf.c \
	w_sinh.c \
	w_sinhf.c \
	w_sqrt.c \
	w_sqrtf.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/libm-i387
MY_FIRST_INCLUDES :=	$(MY_FIRST_INCLUDES_IN)

# Is there a better way to do this?
OBJ_DIR := $(OBJ_DIR_IN)

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-D__NO_MATH_INLINES \
		-D__LIBC_INTERNAL_MATH_INLINES \
		-DASSEMBLER \
		-DGAS_SYNTAX \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	e_acos.S \
	e_acosf.S \
	e_acosh.S \
	e_acoshf.S \
	e_asin.S \
	e_asinf.S \
	e_atan2.S \
	e_atan2f.S \
	e_atanh.S \
	e_atanhf.S \
	e_exp.S \
	e_exp10.S \
	e_exp10f.S \
	e_expf.S \
	e_fmod.S \
	e_fmodf.S \
	e_log.S \
	e_log10.S \
	e_log10f.S \
	e_logf.S \
	e_pow.S \
	e_powf.S \
	e_rem_pio2.c \
	e_rem_pio2f.c \
	e_remainder.S \
	e_remainderf.S \
	e_scalb.S \
	e_scalbf.S \
	e_sqrt.S \
	e_sqrtf.S \
	k_rem_pio2.c \
	k_rem_pio2f.c \
	s_asinh.S \
	s_asinhf.S \
	s_atan.S \
	s_atanf.S \
	s_cbrt.S \
	s_cbrtf.S \
	s_ceil.S \
	s_ceilf.S \
	s_cexp.S \
	s_cexpf.S \
	s_copysign.S \
	s_copysignf.S \
	s_copysignl.S \
	s_cos.S \
	s_cosf.S \
	s_exp2.S \
	s_exp2f.S \
	s_expm1.S \
	s_expm1f.S \
	s_fdim.S \
	s_fdimf.S \
	s_finite.S \
	s_finitef.S \
	s_finitel.S \
	s_floor.S \
	s_floorf.S \
	s_fma.S \
	s_fmaf.S \
	s_fmax.S \
	s_fmaxf.S \
	s_fmin.S \
	s_fminf.S \
	s_frexp.S \
	s_frexpf.S \
	s_frexpl.S \
	s_ilogb.S \
	s_ilogbf.S \
	s_isinfl.c \
	s_isnanl.c \
	s_llrint.S \
	s_llrintf.S \
	s_log1p.S \
	s_log1pf.S \
	s_log2.S \
	s_log2f.S \
	s_logb.S \
	s_logbf.S \
	s_lrint.S \
	s_lrintf.S \
	s_nearbyint.S \
	s_nearbyintf.S \
	s_remquo.S \
	s_remquof.S \
	s_rint.S \
	s_rintf.S \
	s_scalbln.c \
	s_scalblnf.c \
	s_scalbn.S \
	s_scalbnf.S \
	s_scalbnl.S \
	s_significand.S \
	s_significandf.S \
	s_sin.S \
	s_sincos.S \
	s_sincosf.S \
	s_sinf.S \
	s_tan.S \
	s_tanf.S \
	s_trunc.S \
	s_truncf.S \
	t_exp.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/generic
MY_FIRST_INCLUDES :=	$(MY_FIRST_INCLUDES_IN)

# Is there a better way to do this?
OBJ_DIR := $(OBJ_DIR_IN)

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-D__NO_MATH_INLINES \
		-D__LIBC_INTERNAL_MATH_INLINES \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	fpu_control.c \
	setfpucw.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/sysdeps/i386/fpu
MY_FIRST_INCLUDES :=	$(MY_FIRST_INCLUDES_IN)

# Is there a better way to do this?
OBJ_DIR := $(OBJ_DIR_IN)

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-D__NO_MATH_INLINES \
		-D__LIBC_INTERNAL_MATH_INLINES \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	fclrexcpt.c \
	fgetexcptflg.c \
	fraiseexcpt.c \
	fsetexcptflg.c \
	ftestexcept.c \
	fegetround.c \
	fesetround.c \
	fegetenv.c \
	feholdexcpt.c \
	fesetenv.c \
	feupdateenv.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk

SRC_DIR:=src/kit/glibc/math
MY_FIRST_INCLUDES :=	$(MY_FIRST_INCLUDES_IN)

# MY_CFLAGS - Template User provided flags.
MY_CFLAGS := \
		-DPIC \
		-D_LIBC_REENTRANT \
		-D__NO_MATH_INLINES \
		-D__LIBC_INTERNAL_MATH_INLINES \
		-include $(BUILDHOME)/src/kit/glibc/include/libc-symbols.h \

SRCS:= \
	ieee-math.c \
	conj.c \
	cimag.c \
	creal.c \
	cabs.c \
	carg.c \
	conjf.c \
	cimagf.c \
	crealf.c \
	cabsf.c \
	cargf.c \

MY_TRASH_ID := $(word 1, $(SRCS))

include $(SDIR_TEMPLATES)/CompileObjectsTemplate.mk
