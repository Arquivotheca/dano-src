#ifndef DSETWATCHPOINT_H
#define DSETWATCHPOINT_H

#include "HDialog.h"

class BPositionIO;

class DSetWatchpoint : public HDialog {
public:
	DSetWatchpoint(BRect frame, const char *name, window_type type, int flags,
		BWindow *owner, BPositionIO& data);
		
	enum { sResID = 131 };

protected:
	virtual bool OKClicked();
};

#endif
