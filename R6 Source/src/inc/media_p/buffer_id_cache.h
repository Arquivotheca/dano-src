//	buffer_id_cache
//	Simple caching of id->BBuffer look-up since, as long
//	as the buffer exists, it's guaranteed to not change.
//	Not thread safe, because in the uses we make of it,
//	it doesn't have to be since there's only a single
//	caller thread per instance.

#if !defined(buffer_id_cache_h)
#define buffer_id_cache_h

#include <trinity_p.h>

class BBuffer;

class _buffer_id_cache {
		struct buf_cache_ent {
			media_buffer_id id;
			BBuffer * buffer;
		};
		enum {
			MAX_BUF_CACHE = 64		//	power of two!
		};
		_BMediaRosterP * m_roster;
		buf_cache_ent m_bufs[MAX_BUF_CACHE];

public:
		_buffer_id_cache(
				_BMediaRosterP * roster)
			{
				m_roster = roster;
				for (int ix=0; ix<MAX_BUF_CACHE; ix++) {
					m_bufs[ix].id = -1;
					m_bufs[ix].buffer = 0;
				}
			}
		~_buffer_id_cache()
			{
			}
		inline BBuffer * FindBuffer(
				media_buffer_id id)
			{
				int ix = id&(MAX_BUF_CACHE-1);
				if (m_bufs[ix].id != id) {
					m_bufs[ix].buffer = m_roster->FindBuffer(id);
					m_bufs[ix].id = id;
				}
				return m_bufs[ix].buffer;
			}
		inline void FlushBuffer(
				media_buffer_id id)
			{
				m_bufs[id&(MAX_BUF_CACHE-1)].buffer = NULL;
				m_bufs[id&(MAX_BUF_CACHE-1)].id = -1;
			}

static	void * operator new(size_t size);
static	void operator delete(void * ptr);

};

#endif	//	buffer_id_cache_h
