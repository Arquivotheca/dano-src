#ifndef __DWATCHPOINT_H__
#define __DWATCHPOINT_H__

#include "DTracepoint.h"

class DWatchpoint : public DTracepoint {
public:
	DWatchpoint()
		:	fAddress(0)
		{}

	DWatchpoint(const char *name, ptr_t address)
		:	DTracepoint(name),
			fAddress(address)
		{ }
	
	DWatchpoint(const char *name, uint32 kind, ptr_t address)
		:	DTracepoint(name, kind),
			fAddress(address)
		{ }
	
	bool Match(ptr_t address) const
		{ return fAddress == address; }

	ptr_t Address() const
		{ return fAddress; }

	virtual unsigned char *EnabledIconBits() const;

private:
	ptr_t fAddress;
	static unsigned char *sfIcon;
};

#endif

