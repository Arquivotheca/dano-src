//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#ifndef _CAPP_H_
#define _CAPP_H_

#include <app/Application.h>

class CalcApplication : public BApplication {
	public:
							CalcApplication();
	
	void					GetPrefs(void);
	void					SavePrefs(void);
};

#endif
