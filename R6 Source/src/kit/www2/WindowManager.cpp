#include <WindowManager.h>
#include <Messenger.h>
#include <Application.h>
#include <URL.h>
#include <stdio.h>
#include "StringBuffer.h"

using namespace B::WWW2;

WindowManager windowManager;

WindowManager::WindowManager()
{
}

void WindowManager::OpenWindow(const char *name, const BRect &dimensions, const URL &url,
	const BMessenger &respondTo, int32 requestID, const char *paramString)
{
	StringBuffer tmp;
	tmp << url;

	BMessage openWindow('opnw');
	openWindow.AddInt32("width", dimensions.Width());
	openWindow.AddInt32("height", dimensions.Height());
	openWindow.AddInt32("requestID",requestID);
	openWindow.AddString("url", tmp.String());
	openWindow.AddString("name", name);
	openWindow.AddString("params", paramString);
	openWindow.AddMessenger("messenger", respondTo);
	be_app->PostMessage(&openWindow);
}

void WindowManager::OpenWindow(const char *, const BRect &, const URL &,
	BrowserWindowFlags )
{
}

void WindowManager::CloseWindow(const char *name)
{
	BMessage closeWindow('clsw');
	closeWindow.AddString("name", name);
	be_app->PostMessage(&closeWindow);
}

