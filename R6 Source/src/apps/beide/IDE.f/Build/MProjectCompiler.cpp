// ---------------------------------------------------------------------------
/*
	MProjectCompiler.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			13 July 1999

	An MCompile object that compiles entire projects
	Used to compile subprojects during a make
		
*/
// ---------------------------------------------------------------------------

#include "MProjectCompiler.h"
#include "MThread.h"
#include "MSubProjectFileLine.h"
#include "IDEMessages.h"
#include "ProjectCommands.h"
#include "MProjectWindow.h"
#include "MProjectView.h"

#include <Application.h>

// ---------------------------------------------------------------------------
// class MCompileSubProjectThread
// ---------------------------------------------------------------------------

class MCompileSubProjectThread : public MThread
{
public:
							MCompileSubProjectThread(MProjectCompiler* inCompiler);

	virtual	status_t		Execute();

private:
	MProjectCompiler*	fCompiler;
};

// ---------------------------------------------------------------------------

MCompileSubProjectThread::MCompileSubProjectThread(MProjectCompiler* inCompiler)
{
	fCompiler = inCompiler;
}

// ---------------------------------------------------------------------------

status_t
MCompileSubProjectThread::Execute()
{
	// Open the project window specified by the project line
	// Tell it to build (and wait for an answer)
	// Depending on result post an error
	// Clean up the compiler object
	
	entry_ref ref;
	fCompiler->fProjectLine->GetSourceFile()->GetRef(ref);
	BMessage msg(msgOpenProjectAndReply);
	BMessage reply;
	msg.AddRef("refs", &ref);
	be_app_messenger.SendMessage(&msg, &reply);
	status_t errCode = B_ERROR;
	int32 errors = 0;
	errCode = (status_t) reply.FindInt32("status");
	MProjectWindow* projectWindow = nil;
	reply.FindPointer(kProjectMID, (void**) &projectWindow);
	
	if (errCode == B_OK && projectWindow && this->Cancelled() == false) {
		// Now that the project window is opened, do the build
		// using the project window as the target of our message
		fCompiler->SetProjectWindow(projectWindow);
		
		BMessage buildMessage(msgMakeAndReply);
		BMessage buildReply;
		BMessenger projectMessenger(NULL, projectWindow);
		projectMessenger.SendMessage(&buildMessage, &buildReply);
		errCode = (status_t) buildReply.FindInt32("status");
		errors = buildReply.FindInt32("errors");
		// Now that we are done with the subproject, clear our that state
		fCompiler->SetProjectWindow(NULL);
	}

	// This check requires a little explanation...
	// The MCompileGenerator will call MProjectCompiler::Kill and then
	// it will delete the MProjectCompiler object.
	// Meanwhile, my thread (in this method) is sitting around waiting for the
	// project to reply.  If I get to here and the thread has been cancelled
	// I don't want to touch any part of the MProjectCompiler because
	// it has all been deleted.
	
	if (this->Cancelled()) {
		return B_OK;
	}
	
	// It would be nice to let the errors in the subproject speak for themselves
	// (ie: don't pull up the master's error window)
	// However, if we don't then the master continues to compile
	
	// If the user cancels the subproject (but not through the master) then
	// we will get here rather than returning with the check above.
	// Cancel the master project also and then just return.  
	// (Returning without cleaning up then allows the MCompilerGenerator::Kill
	// to work just like normal as if this compile was still busy working.)

	if (errCode == B_CANCELED) {
		fCompiler->fMasterProjectView.Window()->PostMessage(cmd_Cancel);	
		return B_OK;
	}
	else if (errCode != B_OK) {
		ErrorNotificationMessageShort message;
		message.hasErrorRef = false;
		message.isWarning = false;
		char buffer[256];
		
		if (projectWindow == nil) {
			sprintf(buffer, "Could not open sub-project \"%s\"", fCompiler->fProjectLine->Name());
		}
		else if (errCode == B_BUSY) {
			sprintf(buffer, "Sub-project \"%s\" is currently busy.", fCompiler->fProjectLine->Name());
		}
		else {
			sprintf(buffer, "Errors during build of sub-project \"%s\"\nRefer to Errors & Warnings for %s", 
					fCompiler->fProjectLine->Name(), fCompiler->fProjectLine->Name()); 
		}
		strncpy(message.errorMessage, buffer, 256);
		fCompiler->fMasterProjectView.GetErrorMessageWindow()->PostErrorMessage(message);
	}
	
	fCompiler->DoStatus(errCode == B_OK, errCode);
	
	// If we don't have any errors, get back to main project in focus
	if (errCode == B_OK) {
		fCompiler->fMasterProjectView.Window()->Activate();
	}

	// we are done - clean up and delete the compiler object
	if (fCompiler->Lock()) {
		fCompiler->fCompilerDone = true;
		delete fCompiler;
	}
	
	return errCode;
}

// ---------------------------------------------------------------------------
// MProjectCompiler member functions
// ---------------------------------------------------------------------------

MProjectCompiler::MProjectCompiler(MProjectView& inView, MSubProjectFileLine* inProjectLine)
				 : MCompile(), fMasterProjectView(inView)
{
	fProjectLine = inProjectLine;
	fCompileThread = NULL;
	fSubProjectWindow = NULL;
}

// ---------------------------------------------------------------------------

MProjectCompiler::~MProjectCompiler()
{
}


// ---------------------------------------------------------------------------

status_t
MProjectCompiler::Run()
{
	fCompileThread = new MCompileSubProjectThread(this);
	return fCompileThread->Run();
}

// ---------------------------------------------------------------------------

status_t
MProjectCompiler::Kill()
{
	// Terminate the build of the subproject
	status_t status = B_OK;
	
	if (this->Lock()) {
		// Notify my thread that we need to stop (see Execute)
		fCompileThread->Cancel();
	
		// Get the sub project window and tell it to cancel
		if (fSubProjectWindow) {
			fSubProjectWindow->PostMessage(cmd_Cancel);
		}	
		status =  MCompile::Kill();
		this->Unlock();
	}
		
	return status;
}

// ---------------------------------------------------------------------------

void
MProjectCompiler::DoStatus(bool objProduced, status_t errCode)
{
	fProjectLine->CompileDone(errCode, 0, 0, NULL);	
	MCompile::DoStatus(objProduced, errCode);
}

// ---------------------------------------------------------------------------

void
MProjectCompiler::DoStatus(const CompilerStatusNotification& inRec)
{
	MCompile::DoStatus(inRec);
}

// ---------------------------------------------------------------------------

void
MProjectCompiler::SetProjectWindow(MProjectWindow* projectWindow)
{
	this->Lock();
	fSubProjectWindow = projectWindow;
	this->Unlock();
}

