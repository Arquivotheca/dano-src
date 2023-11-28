// ============================================================
//  CMonitorControl.h	©1996 Hiroshi Lockheimer
// ============================================================

#if ! defined CMONITORCONTORL_INCLUDED
#define CMONITORCONTORL_INCLUDED

#include <View.h>
#include <Invoker.h>
#include <Rect.h>

class BMessage;
class BPoint;
class BBitmap;

class CMonitorControl : public BView, public BInvoker {
public:
					CMonitorControl(BRect frame, const char *name,
									BMessage *message, long value);
	virtual			~CMonitorControl();
	
	virtual void	AttachedToWindow();
	virtual void	Draw(BRect updateRect);
	virtual void	MouseDown(BPoint where);
				
	void			SetValue(long inValue);
	long			Value() const;
	
	virtual void	SetEnabled(bool on);
	bool			IsEnabled() const;
	
protected:
	long			mValue;
	bool			mEnabled;
	BBitmap			*mBitmap;
};

inline long
CMonitorControl::Value() const
	{ return (mValue); }
	
inline bool
CMonitorControl::IsEnabled() const
	{ return (mEnabled); }

#endif
