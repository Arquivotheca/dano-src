#ifndef DGETMEMORYLOCATION_H
#define DSETMEMORYLOCATION_H

#include "HDialog.h"

class BPositionIO;

class DGetMemoryLocation : public HDialog {
public:
	DGetMemoryLocation(BRect frame, const char *name, window_type type, int flags,
		BWindow *owner, BPositionIO& data);
	
	void SetDefaultAddress(ptr_t addr);
	
	enum { sResID = 133 };

protected:
	virtual bool OKClicked();
};

#endif
