//******************************************************************************
//
//	File:			WMRootView.cpp
//
//	Description:	WMRootView implementation
//	
//	Written by:		Mathias Agopian
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include "wm.h"
#include "WMRootView.h"

#include <render2/Color.h>

// ********************************************************************

WMRootView::WMRootView(const WindowManager::ptr& content)
	:	BView(),
		lock(BLocker("WMRootView")),
		m_content(content)
{
}

WMRootView::~WMRootView()
{
}

void WMRootView::Draw(const IRender::ptr& into)
{
	if (lock.Lock() == B_OK) {
		const BRect r = Bounds();
		into->Rect(r);
		into->Color(BColor(BColor32(51,102,152,255)));
		lock.Unlock();
	}
}

void WMRootView::DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result)
{
	m_content->DispatchEvent(msg, this);
	if (result)
		*result = B_EVENT_ABSORBED;
}
