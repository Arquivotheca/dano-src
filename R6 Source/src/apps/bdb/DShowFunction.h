#ifndef DSHOWFUNCTION_H
#define DSHOWFUNCTION_H

#include "HDialog.h"

class BPositionIO;

class DShowFunction : public HDialog {
public:
	DShowFunction(BRect frame, const char *name, window_type type, int flags,
		BWindow *owner, BPositionIO& data);

	void SetDefaultName(const char* name);
		
	enum { sResID = 134 };

protected:
	virtual bool OKClicked();
};

#endif
