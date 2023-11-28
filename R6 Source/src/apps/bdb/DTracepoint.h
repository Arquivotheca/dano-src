#ifndef __DTRACEPOINT_H__
#define __DTRACEPOINT_H__

#include <String.h>

class DTracepoint {
	// DBreakpoint or DWatchpoint
public:

	// kDisabled means the user disabled it manually
	// kOnHold means that it currently is unset because it is in an image that
	// hasn't been loaded.  It is "on-hold" waiting for a image loaded call.
	
	enum {
		kPlain = 0,
		kSingleShot = 0x1,
		kConditional = 0x2,
		kCounted = 0x4,
		kActionPoint = 0x8,
		kDisabled = 0x10,
		kOnHold = 0x20,
		kAll = 0xffffffff
	};
	
	DTracepoint()
		:	fKind(0)
		{}

	DTracepoint(const char *name)
		:	fName(name),
			fKind(0)
		{}
	DTracepoint(const char *name, uint32 kind)
		:	fName(name),
			fKind(kind)
		{}
	virtual ~DTracepoint() {}

	const char *Name() const
		{ return fName.String(); }
	void SetName(const char *name)
		{ fName = name; }
	
	bool SingleShot() const
		{ return fKind & kSingleShot; }
	void SetSingleShot(bool on)
		{
			if (on)
				fKind |= kSingleShot;
			else
				fKind &= ~kSingleShot;
		}
	
	bool OnHold() const
		{ return fKind & kOnHold; }
	void SetOnHold(bool on)
		{
			if (on)
				fKind |= kOnHold;
			else
				fKind &= ~kOnHold;
		}
			
	bool Counted() const
		{ return fKind & kCounted; }
	void SetCounted(bool on)
		{
			if (on)
				fKind |= kCounted;
			else
				fKind &= ~kCounted;
		}
	int32 Count() const
		{ return fCount; }
	void SetCount(int32 count)
		{ fCount = count; }

	bool Disabled() const
		{ return fKind & kDisabled; }
	void SetDisabled(bool on)
		{
			if (on)
				fKind |= kDisabled;
			else
				fKind &= ~kDisabled;
		}

	uint32 Kind() const
		{
			return fKind;
		}
		
	virtual void Draw(BView *, BPoint, uint32);
	virtual bool Track(BView *, BPoint , bool);
	virtual unsigned char *EnabledIconBits() const = 0;
	
protected:
	BString fName;
	uint32 fKind;
	// ... add condition
	int32 fCount;
	// ... add action
};

#endif
