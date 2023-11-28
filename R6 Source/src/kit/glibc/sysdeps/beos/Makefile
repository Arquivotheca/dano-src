ifeq ($(subdir),misc)
sysdep_routines += beos-sysdep
	sysdep_header = dlfcn.h bits/Errors.h bits/stdio-lock.h
endif

ifeq ($(subdir),stdio-common)
routines += siglist
inhibit-siglist = yes
endif

ifeq ($(subdir),stdlib)
routines += alloca
endif

inhibit-stdio_lim = yes
