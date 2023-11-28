// ===========================================================================
//	CAppletBuddy.h				c1996-1997 Metrowerks Inc. All rights reserved.
//
//								Author:  	Burton T. E. Miller
//								Date:		10/97
// ===========================================================================

#pragma once

#pragma export on

#include "MWAppletMonitor.h"

class MWAppletContextView;

// ===========================================================================
//		* MWAppletBuddy
// ===========================================================================
//	This class is used to contain java applets.  Create it and put it into a
// view heirarchy, as you would any view.  Then ask it to display an applet.

class MWAppletBuddy
{
public:							
	// Constructor - Call with a valid URL string, and preferably with a
	// valid applet tag (including params - the whole thing).  If the applet
	// tag is not included, then the FIRST tag will be extracted from the
	// URL.  An invalid URL or URL/tag combination will result in an invalid
	// MWAppletBuddy object and associated view (no applets will display).
	// This invalidity will not be apparent until the view is attached to a
	// window.
	// Also, a derivative of MWAppletMonitor can be passed in to provide
	// feedback on the applet's status.  This object is NOT owned by this
	// class, and should not be destructed until the object into which it
	// is passed is destructed.  One object may be passed into several
	// MWAppletBuddies, but synchronization then becomes an issue - it is ok
	// to use one monitor for all the MWAppletBuddies in a window - because
	// it is guaranteed that all callbacks to your monitor will be in the
	// window thead with which the MWAppletBuddy is associated.
							MWAppletBuddy
								(BRect frame,
								 const char *name,
								 BWindow * window,
								 const char * url,
								 const char * appletTag = NULL,
								 MWAppletMonitor * monitor = NULL,
								 uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
								 uint32 flags = B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	virtual 					~MWAppletBuddy();
	
	// View - This function returns the view associated with an MWAppletBuddy.
	// This view should be attached to a window normally, and should be placed
	// at the coordinates indicated by the applet tag - the view may be
	// resized, but will resize itself to the specs in the tag when the 
	// applet is initialized.
	virtual BView *			View();
	
	// StartJava - Sets up java for use in this application.  This function
	// should be called before any MWAppletBuddy objects are created - a
	// good place is the Application constructor or main.
	// StopJava - Shuts java down - Call after all MWAppletBuddies have been
	// destructed and the MW_APPLET_FINISHED message has been recieved for
	// each MWAppletBuddy - a good place is the Application destructor or main.
	static void				StartJava();
	static void				StopJava();
	
	// AttachWindow/DetachWindow - These functions can be used to insure that
	// java runs efficiently for a given window.  Call AttachWindow before 
	// creating any MWAppletBuddies are created, and call DetachWindow when
	// you know you won't want any more new MWAppletBuddies - QuitRequested is
	// a good place.  These functions are totally OPTIONAL - if you don't call
	// them, then portions of the Java environment may be periodically
	// reloaded, causing slow-downs.
	static void				AttachWindow
								(BWindow * window);
	static void				DetachWindow
								(BWindow * window);

	// All of the following functions work on a valid MWAppletBuddy.  If you
	// construct an MWAppletBuddy with a bad URL or call CloseApplet - it 
	// becomes invalid.  Calls to an invalid MWAppletBuddy have no effect.
	
	// StartApplet - Starts a stopped applet.  Applets are automatically
	// loaded and started initially - call this only after StopApplet.
	// Inverse of StopApplet();
	void						StartApplet();
	// StopApplet - Call this on running applets.  Inverse of StartApplet().
	void 					StopApplet();
	// CloseApplet - Call this if you want an applet to permenantly stop.
	// Automatically called when an MWAppletBuddy's view is detached from a
	// window.  Inverse of applet construction.
	void						CloseApplet();
	// RestartApplet - Call this to restart a running or stopped applet.
	void						RestartApplet();
	// ReloadApplet - Call this to reload a running or stopped applet.
	void						ReloadApplet();
	
	// Returns the MW_APPLET_BUDDY field - hack to work around intel export bugs
	static const char * 		GetMsgFieldName();
	
	static const char * MW_APPLET_BUDDY;

private:
friend class MWAppletContextView;
							MWAppletBuddy
								(const MWAppletBuddy&);

	void						Inform
								(int code);

	BWindow *				mWindow;
	MWAppletContextView *	mAppletContextView;
};				

const uint32 MW_APPLET_FINISHED = 'mwaf';

#pragma export reset

