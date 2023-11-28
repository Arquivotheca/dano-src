/*
	MyError.c
	
	Copyright 1997, Hekkelman Programmatuur
	
	Part of Sum-It for the BeBox version 1.1.

*/

#include <stdarg.h>
#if !__BEOS__
#include "BeCompat.h"
#endif

#include "HError.h"

#include <Beep.h>
#include <Alert.h>

#include <stdio.h>

HErr::HErr()
{
} /* HErr::HErr */

HErr::HErr(int err, ...) throw()
{
	fErr = err;
	fMessage[0] = 0;
	
	if (err == 0)
		strcpy(fMessage, "No Error");
	else if (err < 0)
		strcpy(fMessage, strerror(err));
	else
		sprintf(fMessage, "An unknown error occured (nr: %d)", err);

//#if DEBUG
//	if (err)
//	{
//#if __BEOS__
//		SERIAL_PRINT(("%s\n", fMessage));
//#else
//		Str255 msg;
//		c2pstrcpy(msg, fMessage);
//		DebugStr(msg);
//#endif
//	}
//#endif
} /* HErr::HErr */

HErr::HErr(const char *errString, ...) throw()
{
	va_list vl;
	va_start(vl, errString);
	vsprintf(fMessage, errString, vl);
	va_end(vl);
	fErr = -1;
	
//#if DEBUG
//#if __BEOS__
//	SERIAL_PRINT(("%s\n", fMessage));
//#else
//	Str255 msg;
//	c2pstrcpy(msg, fMessage);
//	DebugStr(msg);
//#endif
//#endif
} // HErr::HErr

void HErr::DoError () throw ()
{
	if (fErr)
	{
		beep();
		(new BAlert("", fMessage, "OK"))->Go();
	}
} /* HErr::DoError */

#if !__BEOS__
void _AssertionFailure_(const char *cond, const char *file, int line)
{
	char b[1024];
	sprintf(b, "Assertion failed: %s (File: \"%s\", Line: %d)", cond, file, line);
	syslog(LOG_DEBUG, "%s", b);
	DebugStr(c2pstr(b));
} // _AssertionFailure_
#endif
