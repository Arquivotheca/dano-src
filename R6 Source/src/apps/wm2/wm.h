//******************************************************************************
//
//	File:			wm.h
//
//	Description:	WindowManager
//	
//	Written by:		Mathias Agopian
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef _WM_H_
#define _WM_H_

#include <content2/Content.h>

#include <interface2/View.h>

#include <support2/Locker.h>
#include <support2/Message.h>
#include <support2/StdIO.h>
#include <support2/SupportDefs.h>
#include <support2/Value.h>

using namespace B::Content2;
using namespace B::Interface2;
using namespace B::Support2;

class WMRootView;

// ********************************************************************

class WindowManager : public BContent
{
public:
		B_STANDARD_ATOM_TYPEDEFS(WindowManager)

						WindowManager();
		virtual			~WindowManager();
		IView::ptr 		CreateView(const BMessage &attr);

		virtual	void 	DispatchEvent(const BMessage& msg, const IView::ptr& view);
		virtual	void 	KeyDown(const BMessage& msg, const char *bytes, int32 numBytes);
		virtual	void 	KeyUp(const BMessage& msg, const char *bytes, int32 numBytes);
		virtual	BValue	Inspect(const BValue &which, uint32 flags = 0);

private:
	atom_ref<WMRootView>	m_view;
	BNestedLocker			m_lock;
};


#endif
