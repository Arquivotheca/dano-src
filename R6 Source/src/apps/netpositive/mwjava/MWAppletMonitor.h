// ===========================================================================
//	CAppletMonitor.h			c1996-1997 Metrowerks Inc. All rights reserved.
//
//								Author:  	Burton T. E. Miller
//								Date:		11/97
// ===========================================================================

#pragma once

#pragma export on

// ===========================================================================
//		* MWAppletMonitor
// ===========================================================================
// Defines the interface used to monitor applets.

enum MWAppletState
{
	MW_ERROR = -1,
	MW_UNINITIALIZED = 0,
	MW_INITIALIZED = 1,			// applet has been successfully initialized
	MW_FINISHED = 2				// applet is completely finished running
};

class MWAppletMonitor
{
public:
	
	virtual void ShowStatus
		(const char * status) = 0;
	virtual void ShowDoc
		(const char * url,
		 const char * target) = 0;
	virtual void ReportState
		(MWAppletState state) = 0;
};

#pragma export reset

