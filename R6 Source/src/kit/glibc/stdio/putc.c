#include <stdio.h>
#undef	putc
#define	fputc	putc
#define fputc_unlocked	putc_unlocked
#include <fputc.c>
