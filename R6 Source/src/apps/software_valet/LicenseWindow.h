// LicenseWindow.h
#ifndef _LICENSEWINDOW_H_
#define _LICENSEWINDOW_H_

#include <Window.h>

class LicenseWindow : public BWindow
{
public:
	LicenseWindow(char *text, bool *, size_t, char *styles);

virtual bool QuitRequested();
private:
	bool	*canceled;
};

#endif

