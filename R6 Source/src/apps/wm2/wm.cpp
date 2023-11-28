//******************************************************************************
//
//	File:			wm.cpp
//
//	Description:	WindowManager implementation
//	
//	Written by:		Mathias Agopian
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include "wm.h"
#include "WMRootView.h"

// ********************************************************************

WindowManager::WindowManager()
	: 	BContent(),
		m_lock(BNestedLocker("WindowManager"))
{
}

WindowManager::~WindowManager()
{
}

IView::ptr WindowManager::CreateView(const BMessage &attr)
{
	(void)attr;
	
	// We can't be asked a rootview twice
	IView::ptr view = m_view.promote();
	if (view != NULL)
		return NULL;

	view = new WMRootView(this);
	m_view = static_cast<WMRootView *>(view.ptr());
	return view;
}

void WindowManager::DispatchEvent(const BMessage& msg, const IView::ptr& view)
{
}

void WindowManager::KeyDown(const BMessage& msg, const char *bytes, int32 numBytes)
{
}

void WindowManager::KeyUp(const BMessage& msg, const char *bytes, int32 numBytes)
{
}

BValue WindowManager::Inspect(const BValue &which, uint32 flags)
{
	return LContent::Inspect(which, flags)
//		.Overlay(LWindowManager::Inspect(which, flags));
}

