#ifndef _WINDOW_MANAGER_H
#define _WINDOW_MANAGER_H

#include <SupportKit.h>

class BRect;
class BMessenger;

namespace Wagner {

class URL;


class WindowManager {
public:
	WindowManager();
	void OpenWindow(const char *name, const BRect &dimensions, 	const URL &url,
		const BMessenger &respondTo, int32 requestID, const char *paramString);
	void CloseWindow(const char *name);
private:
	enum BrowserWindowFlags {
		SHOW_TOOLBAR = 1
	};
	void OpenWindow(const char *name, const BRect &dimensions, 	const URL &url,
		BrowserWindowFlags flags);
};

};

extern Wagner::WindowManager windowManager;

#endif
