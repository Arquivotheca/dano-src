/*
 * Copyright 2000 Be, Incorporated.  All Rights Reserved.
 */

#ifndef _ATTRSETTINGS_H_
#define _ATTRSETTINGS_H_

#include "Settings.h"

class attrSF : public SettingsFile {
public:
	
	attrSF(const char *name);
	~attrSF();

	virtual status_t Load(int32 flags = S_NONE);
	virtual status_t Save(int32 flags = S_NONE);
};


#endif	/* _ATTRSETTINGS_H_ */
