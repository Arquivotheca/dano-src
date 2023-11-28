/*
	Debug.cpp
	
	Copyright 1994 Be, Inc. All Rights Reserved.
	
*/

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#ifndef _SUPPORT_DEFS_H
#include <SupportDefs.h>
#endif

#ifndef _OS_H
#include <OS.h>
#endif

#ifndef _PRIV_SYSCALLS_H
#include <priv_syscalls.h>
#endif

#ifndef _STDARG_H
#include <stdarg.h>
#endif

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#endif

#ifndef _STRING_H
#include <string.h>
#endif

static FILE *fpDebug = stdout;

_EXPORT bool _rtDebugFlag = TRUE;

bool _debugFlag()
{
	return _rtDebugFlag;
}

bool _setDebugFlag(bool newValue)
{
	bool oldValue = _rtDebugFlag;
	_rtDebugFlag = newValue;
	return oldValue;
}

sem_id __print_sem__ = 0;


int _xdebugPrintf(const char *format, ...)
{
	va_list args;
	int		res;
	
	va_start (args, format);
	res = vfprintf (fpDebug, format, args);	
	va_end (args);
	return res;
}


int _sPrintf(const char * format, ...)
{
	va_list 	args;
	char		s[256];
	int			res;
	
	if (_rtDebugFlag) {
		if (__print_sem__ == 0)
			__print_sem__ = create_sem(1, "sdfsdsgd");
		
		while (acquire_sem(__print_sem__) == B_INTERRUPTED)
			;
		va_start (args, format);
#if __GLIBC__
		res = vsnprintf (s, 256, format, args);	
#else
#error
		res = vsprintf (s, format, args);	
#endif
		_kdprintf_(s);
		va_end (args);
		release_sem(__print_sem__);
		return res;
	} else
		return 0;
}

int _debugPrintf(const char * format, ...)
{
	if (_rtDebugFlag) {
		va_list args;
		int	rval;
		
		if (__print_sem__ == 0)
			__print_sem__ = create_sem(1, "sdfsdsgd");
		
		while (acquire_sem(__print_sem__) == B_INTERRUPTED)
			;
		va_start(args, format);
		rval = (vfprintf (fpDebug, format, args));
		va_end (args);
		release_sem(__print_sem__);
		return rval;
	} else {
		return 0;
	}
}

int _debuggerAssert(const char *file, int line, char *expr)
{
	char *format = "Assert failed: File: %s, Line: %d, Thread: %s.\nAssert condition: %s\n";
	
	const char* thread_name="<unknown>";
	
	thread_info thread;
	if (get_thread_info(find_thread(NULL), &thread) == B_OK)
		thread_name = thread.name;
	
	int len = strlen(expr) + strlen(file) + strlen(format)
			+ strlen(thread_name) + 30;
	
	char *buf = (char *) malloc(len);
	
	sprintf(buf, format, file, line, thread_name, expr);
	_sPrintf("%s", buf);
	debugger(buf);
	free(buf);
	
	return 0;
}
