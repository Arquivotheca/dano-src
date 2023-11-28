// ---------------------------------------------------------------------------
/*
	MProjectCompiler.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			13 July 1999

	An MCompile object that compiles entire projects
	Used to compile subprojects during a make
		
*/
// ---------------------------------------------------------------------------

#ifndef _MPROJECTCOMPILER_H
#define _MPROJECTCOMPILER_H

#include "MCompile.h"

class MSubProjectFileLine;
class MProjectView;
class MCompileSubProjectThread;
class MProjectWindow;

class MProjectCompiler : public MCompile
{
public:
								MProjectCompiler(MProjectView& inView,
												 MSubProjectFileLine* inProjectLine);
	virtual						~MProjectCompiler();

	virtual	status_t			Run();
	virtual	status_t			Kill();

protected:
	// This can be overridden, but be sure to call inherited!
	virtual	void				DoStatus(bool objProduced,
										 status_t errorCode);
	virtual	void				DoStatus(const CompilerStatusNotification& inRec);

private:
	friend class MCompileSubProjectThread;
	
	MSubProjectFileLine*		fProjectLine;
	MProjectView&				fMasterProjectView;
	MCompileSubProjectThread*	fCompileThread;
	MProjectWindow*				fSubProjectWindow;

	void						SetProjectWindow(MProjectWindow* projectWindow);
	
};

#endif
