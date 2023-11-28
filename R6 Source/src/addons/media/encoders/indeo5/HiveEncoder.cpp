extern "C" {
	#include <datatype.h>
	#include <pia_main.h>
}
#include <stdio.h>
#include <stdarg.h>

void __cdecl	HivePrintF( PChr f, ... )
{
	va_list         args;
	va_start(args,f);
	vprintf(f, args);
	va_end(args);
}

void	__cdecl	HivePrintString( PChr s)
{
	printf("%s", s);
}

void	__cdecl HivePrintHexInt( U32 uValue )
{
	printf(" 0x%x", uValue);
}

void	__cdecl HivePrintDecInt( I32 iValue )
{
	printf(" %d", iValue);
}
