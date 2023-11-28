
#if !defined(_miniautolock_h)
#define _miniautolock_h

#include "minilocker.h"

class MiniAutolock {
public:
		MiniAutolock(MiniLocker & l) : fL(l) { isLocked = fL.Lock(); if (!isLocked) debugger("not locked!"); }
		~MiniAutolock() { if (isLocked) fL.Unlock(); }
		bool IsLocked() { return isLocked; }
private:
		bool isLocked;
		MiniLocker & fL;
};


#endif // _miniautolock_h
