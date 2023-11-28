/*	tr_debug.h	*/

#if !defined(tr_debug_h)
#define tr_debug_h

#if !defined(_DEBUG_H)
 #include <Debug.h>
#endif

namespace BPrivate {
	class _dlogger {
	public:
		bigtime_t when;
		int32 thread;
		int32 line;
		char file[32];
		char msg[80];
	static port_id log_port;
	static bool _check_fail(const char *file, int line, const char * msg);
		_dlogger();
		_dlogger(const char * file, int line);
		void message(const char * fmt, ...);
	};
}
using namespace BPrivate;

#if DEBUG
	#define dlog _dlogger(__FILE__,__LINE__).message
	#define dassert(x) do { if (!(x)) dlog("ASSERT FAILED: " #x); } while(0)
	#define dcheck(x) ((x) ? true : _dlogger::_check_fail(__FILE__,__LINE__,#x))

#if DEBUG_BUFFER_REFS
	void debug_attach();
	void debug_request_buf(int id);
	void debug_got_buf(int id);
	void debug_sent_buf(int id);
	void debug_recycled_buf(int id);
	void debug_dump_bufs();
#endif

#else
	#define dlog(x...)
	#define dassert(x) 
	#define dcheck(x) (x)
#endif

#if !DEBUG || !DEBUG_BUFFER_REFS
	#define debug_attach()
	#define debug_request_buf(x)
	#define debug_got_buf(x)
	#define debug_sent_buf(x)
	#define debug_recycled_buf(x)
	#define debug_dump_bufs()
#endif

#endif /* tr_debug_h */
