# Linux/m68k uses Motorola asm syntax and the ELF format.

m68k-syntax-flag = -DMOTOROLA_SYNTAX

ifeq ($(subdir),misc)
sysdep_routines += mremap
endif

ifeq ($(subdir),elf)
sysdep-others += lddlibc4
install-bin += lddlibc4
endif
