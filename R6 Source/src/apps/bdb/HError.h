/*
	MyError.h
	
	Copyright 1997, Hekkelman Programmatuur
	
	Bevat macros die handig zijn bij error checking
*/

#ifndef HERROR_H
#define HERROR_H

#include <string.h>
#include <cerrno>
#include <syslog.h>

#include <Debug.h>

#if DEBUG
#	if !__BEOS__
		void _AssertionFailure_(const char *cond, const char *file, int line);
#		define ASSERT(x)	do { if (!(x)) _AssertionFailure_(#x, __FILE__, __LINE__); } while (false)
#	endif
#	define ASSERT_OR_THROW(x)		ASSERT(x); if (!(x)) throw HErr("Assertion failed at %s:%d: %s", __FILE__, __LINE__, #x)
#else
#	define ASSERT_OR_THROW(x)		if (!(x)) throw HErr("Assertion failed at %s:%d: %s", __FILE__, __LINE__, #x)
#endif

const char kDefaultMessage[] = "An OS error occurred: %s";

class HErr
{
  protected:
	HErr();
  public:
	HErr(int err, ...) throw();
	HErr(const char *msg, ...) throw();
	
	virtual void	DoError() throw();
	
	operator int()	{ return fErr; };
	operator char*() { return fMessage; };
	
  protected:
	long fErr;
	char fMessage[256];
};

#if __BEOS__
//#	define	THROW(args)				do { _sPrintf("exception thrown at File: \"%s\"; Line: %d\n", __FILE__, __LINE__); throw HErr args; } while (false)
//#	define	THROW(args)				do { syslog(LOG_DEBUG, "exception thrown at File: \"%s\"; Line: %d", __FILE__, __LINE__); throw HErr args; } while (false)
#	define	THROW(args)				do { HErr __err_obj args; syslog(LOG_DEBUG, "Throwing: \"%s\" at %s:%d", (char *)__err_obj, __FILE__, __LINE__); throw __err_obj; } while (false)
#else	
#	define THROW(args)				throw HErr args
#endif

#define FailNil(p)								do {	if ((p) == NULL) 										THROW(("Insufficient memory"));	} while (false)
#define FailNilMsg(p,msg)						do {	if ((p) == NULL)										THROW((msg));	} while (false)
#define FailNilRes(p)							do {	if ((p) == NULL)										THROW(("Missing Resource"));	} while (false)
#define FailNilResMsg(p,msg)				do {	if ((p) == NULL)										THROW((msg));	} while (false)
#define FailOSErr(err)							do {	status_t __err = (err); if (__err != B_OK)		THROW((kDefaultMessage, strerror(__err))); } while (false)
#define FailOSErrMsg(err,msg)				do {	status_t __err = (err); if (__err != B_OK)		THROW((msg, strerror(__err))); } while (false)
#define FailOSErr2(err)						do {	status_t __err = (err); if (__err < B_OK)		THROW((kDefaultMessage, strerror(__err))); } while (false)
#define FailOSErr2Msg(err,msg)				do {	status_t __err = (err); if (__err < B_OK)		THROW((msg, strerror(__err))); } while (false)
#define FailIOErr(err)							do {	status_t __err = (err); if (__err < B_OK)		THROW((kDefaultMessage, strerror(__err))); } while (false)
#define FailIOErrMsg(err,msg)				do {	status_t __err = (err); if (__err < B_OK)		THROW((msg, strerror(__err))); } while (false)
#define FailPErr(err)							do {	status_t __err = (err); if (__err < 0)				THROW((kDefaultMessage, strerror(errno))); } while (false)
#define CheckedWrite(stream,data,size)	do {	if (stream.Write(data, size) < size)					THROW(("A write error occurred: %s", strerror(errno)));	} while (false)
#define CheckedRead(stream,data,size)	do {	if (stream.Read(data, size) < size)					THROW(("A read error occurred: %s", strerror(errno)));	} while (false)
#define CheckedRead2(stream,data)		do {	if (stream.Read(&data, sizeof(data)) < sizeof(data))		THROW(("A read error occurred: %s", strerror(errno)));	} while (false)
#define FailMessageTimedOutOSErr(err)	do {	status_t __err = (err); if (__err != B_OK)	{ if (__err == B_WOULD_BLOCK || __err == B_TIMED_OUT) syslog(LOG_DEBUG, "File %s; Line %d; # SendMessage dropped\n", __FILE__, __LINE__); else THROW((kDefaultMessage, strerror(__err))); } } while (false)

#endif // HERROR_H
