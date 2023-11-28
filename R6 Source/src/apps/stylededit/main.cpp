// ============================================================
//  StyledEdit main.cpp	©1996 Hiroshi Lockheimer
// ============================================================

#include "CStyledEditApp.h"


int
main()
{
	CStyledEditApp *theApp = new CStyledEditApp();
	theApp->Run();
	delete (theApp);

    return 0;
}
