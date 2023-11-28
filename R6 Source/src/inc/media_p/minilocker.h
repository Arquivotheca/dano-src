
#if !defined(_minilocker_h)
#define _minilocker_h

#include <OS.h>

class MiniLocker {
public:
		MiniLocker(const char * name);
		~MiniLocker();
		bool Lock();
		void Unlock();
private:
		int32 sem_cnt;
		int32 lock_nest;
		thread_id owner;
		sem_id sem;
};

#endif	//	_minilocker_h

