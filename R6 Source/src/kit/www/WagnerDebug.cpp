
#include <Autolock.h>
#include <Debug.h>
#include <OS.h>
#include <WagnerDebug.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

WDebug wdebug;

#define LOG_SIZE (16*1024)

WDebug::WDebug()
{
	system_info si;
	get_system_info(&si);
	m_maxThreads = si.max_threads;
	m_threadInfo = (ThreadInfo*)malloc(sizeof(ThreadInfo) * m_maxThreads);
	for (int32 i=0;i<m_maxThreads;i++) {
		m_threadInfo[i].traceLevel = 0;
		m_threadInfo[i].thid = 0;
	};
	
	m_log = m_logStart = m_logPos = new char[LOG_SIZE];
}

WDebug::~WDebug()
{
	free(m_threadInfo);
	delete[] m_log;
}

void WDebug::SetTracing(thread_id thid, int32 level)
{
	ThreadInfo &tinfo = m_threadInfo[thid%m_maxThreads];
	tinfo.traceLevel = level;
}

bool WDebug::Trace(int32 level, const char *file, int32 line, const char *function)
{
	int32 me = find_thread(NULL);
	ThreadInfo &tinfo = m_threadInfo[me%m_maxThreads];
	if (tinfo.thid != me) {
		tinfo.traceLevel = 0;
		tinfo.thid = me;
	};

	if (level <= tinfo.traceLevel) {
		printf("thid(%ld): %s:%ld in %s\n",find_thread(NULL),file,line,function);
		return true;
	};
	return false;
}

void WDebug::Log(const char* format, ...)
{
	char buffer[1024];
	
	va_list vl;
	va_start(vl, format);
	vsnprintf(buffer, sizeof(buffer), format, vl);
	buffer[sizeof(buffer)-1] = 0;
	PRINT(("LOG: %s", buffer));
	va_end(vl);
	
	LogRaw(buffer);
}

void WDebug::LogRaw(const char* data, ssize_t length)
{
	if (length < 0) length = strlen(data);
	
	BAutolock l(&m_logLock);
	
	const size_t N = (size_t)(m_log+LOG_SIZE - m_logPos);
	bool moveStart = false;
	if (length <= N) {
		PRINT(("(%p-%p): Copying %ld log chars to %p\n",
				m_log, m_log+LOG_SIZE, length, m_logPos));
		memcpy(m_logPos, data, length);
		if (m_logStart > m_logPos && m_logStart <= m_logPos+length) {
			moveStart = true;
		}
		m_logPos += length;
	} else {
		PRINT(("(%p-%p): Copying %ld log chars to %p\n",
				m_log, m_log+LOG_SIZE, N, m_logPos));
		if (m_logStart > m_logPos) moveStart = true;
		memcpy(m_logPos, data, N);
		memcpy(m_log, data+N, length-N);
		PRINT(("(%p-%p): Copying remaining %ld log chars to %p\n",
				m_log, m_log+LOG_SIZE, length-N, m_log));
		m_logPos = m_log + (length-N);
		if (m_logStart <= m_logPos) moveStart = true;
	}
	
	if (moveStart) {
		PRINT(("Old log start: %p\n", m_logStart));
		m_logStart = m_logPos;
		do {
			m_logStart++;
			if (m_logStart >= (m_log+LOG_SIZE)) m_logStart = m_log;
		} while (*m_logStart != '\n' && m_logStart != m_logPos);
		PRINT(("Old log start: %p\n", m_logStart));
	}
}

void WDebug::ReadLog(BString* out) const
{
	BAutolock l(const_cast<BLocker*>(&m_logLock));
	
	if (m_logStart < m_logPos) {
		const size_t N = (size_t)(m_logPos-m_logStart);
		if (N == 0) {
			*out = "";
			return;
		}
		
		char* buffer = out->LockBuffer(N);
		memcpy(buffer, m_logStart, N);
		out->UnlockBuffer(N);
	
	} else {
		const size_t SN = (size_t)(m_log+LOG_SIZE-m_logStart);
		const size_t N = (size_t)(m_logPos-m_log + SN);
		if (N == 0) {
			*out = "";
			return;
		}
		
		char* buffer = out->LockBuffer(N);
		memcpy(buffer, m_logStart, SN);
		memcpy(buffer+SN, m_log, N-SN);
		out->UnlockBuffer(N);
		
	}
}
