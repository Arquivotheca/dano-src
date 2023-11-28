
#ifndef _WAGNER_DEBUG
#define _WAGNER_DEBUG

#include <Locker.h>
#include <OS.h>
#include <String.h>
#include <SupportDefs.h>

struct ThreadInfo {
	int32 traceLevel;
	int32 thid;
	uint32 padding[3];
};

class WDebug {

	private:

		int32			m_maxThreads;
		ThreadInfo *	m_threadInfo;
		
		BLocker			m_logLock;
		char*			m_log;
		char*			m_logStart;
		char*			m_logPos;

	public:
	
						WDebug();
						~WDebug();

		void 			SetTracing(thread_id thid, int32 level);
		bool		 	Trace(int32 level, const char *file, int32 line, const char *function);
		
		void			Log(const char* format, ...);
		void			LogRaw(const char* data, ssize_t length=-1);
		void			ReadLog(BString* out) const;
};

extern WDebug wdebug;

#define WTRACE(a) wdebug.Trace(a,__FILE__,__LINE__,__FUNCTION__)
#define WLOG wdebug.Log

#endif
