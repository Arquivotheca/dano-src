/* emacs: -*- C++ -*- */
/*
 * Copyright (c) 1996 Be, Inc.	All Rights Reserved 
 */

#ifndef __PMAPP_H__
#define __PMAPP_H__
#include <Application.h>
#include "pmwin.h"
#include "pmlog.h"
#include "pmprefs.h"


class PMApplication : public BApplication {
	PMWindow *win;
public:
	PMApplication(PMPrefs *settings, char *dirname, bool hide=false);
	virtual void AboutRequested(void);
	void MessageReceived(BMessage *message);
};

extern PMLog *logger;
#endif
