//******************************************************************************
//
//	File:			WMRootView.h
//
//	Description:	Window Manager's rootview definition
//	
//	Written by:		Mathias Agopian
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef _WM_ROOTVIEW_H_
#define _WM_ROOTVIEW_H_

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

class WindowManager;

// ********************************************************************

class WMRootView : public BView
{
public:
		B_STANDARD_ATOM_TYPEDEFS(WMRootView)

						WMRootView(const atom_ptr<WindowManager>& content);
		virtual			~WMRootView();
		virtual	BView 	*Copy() { return this; }	
		virtual void	Draw(const IRender::ptr& into);
		virtual	void	DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result);

public:
	// This lock is held while drawing
	BLocker						lock;

private:
	atom_ptr<WindowManager>		m_content;
};


#endif
