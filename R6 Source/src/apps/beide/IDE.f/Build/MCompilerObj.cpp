//========================================================================
//	MCompilerObj.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>

#include "MCompilerObj.h"
#include "MSourceFileLine.h"
#include "MProjectView.h"
#include "MBuildCommander.h"
#include "MMessageWindow.h"
#include "MFileCache.h"
#include "MAlert.h"
#include "ErrorMessage.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "Utils.h"

#include <Application.h>

// ---------------------------------------------------------------------------
//		MCompilerObj
// ---------------------------------------------------------------------------

MCompilerObj::MCompilerObj(
	MSourceFileLine* inLine,
	const char * 	compiler,
	const char *	file,
	BList&			 args,
	bool 			IDEAware,
	MProjectView& 	inProjectView,
	MakeActionT		inAction)
	: MCompileTool(
		compiler,
		args,
		IDEAware,
		inProjectView.BuildCommander().GetPriority()),
	fSourceFileLine(inLine),
	fProjectView(inProjectView),
	fHeaderFileID(0),
	fCompileKind(inAction)
{
	if (inLine == nil && file != nil)
	{
		const char*		filename = strrchr(file, '/');
		if (filename)
		{
			fFileName = ++filename;
		}
	}
	
	fSourceFileCalled = false;
}

// ---------------------------------------------------------------------------
//		FindHeader
// ---------------------------------------------------------------------------
//	Return the headerquery indicating the location of the header file.
//	We keep a list of already included MSourceFile objects that refer
//	to the header files that have been included.  All of these sourceFile
//	objects are owned by the fAllFilesList in the ProjectView.

status_t
MCompilerObj::FindHeader(
	const HeaderQuery& 	inQuery,
	HeaderReply& 		outReply)
{
	MSourceFile*		sourceFile;
	bool				found = false;
	status_t			err = B_NO_ERROR;

#ifdef MWBROWSER
	outReply.recordbrowseinfo = false;
	outReply.browserfileID = 2;
#endif

	// First search through our list of already included header files
	// to see if the file was already included
	int32		index;

	if (fHeaderFileList.FindItem(inQuery.fileName, inQuery.inSysTree, index))
	{
		outReply.errorCode = B_NO_ERROR;
		outReply.alreadyIncluded = true;
		found = true;
		sourceFile = (MSourceFile*) fHeaderFileList.ItemAt(index);
		err = sourceFile->GetPath(outReply.filePath, MAX_PATH_LENGTH) > B_NO_ERROR ? B_NO_ERROR : err;
	}

	// Wasn't already included
	if (! found && err == B_NO_ERROR)
	{
		bool		foundInSystemTree;

		err = fProjectView.FindHeader(inQuery, outReply, sourceFile, foundInSystemTree);
		if (err == B_NO_ERROR)
		{
			if (foundInSystemTree != inQuery.inSysTree && 
				fHeaderFileList.FindItem(inQuery.fileName, foundInSystemTree, index))
			{
				outReply.alreadyIncluded = true;
			}
			else
			{
				fHeaderFileList.AddItem(sourceFile);		// Add to already-included list
				outReply.alreadyIncluded = false;
			}

			outReply.errorCode = B_NO_ERROR;
			found = true;
		}
	}

	DoneWithLastFile();

	outReply.handle.id = -1;
	if (err == B_NO_ERROR)
	{
		if (fFileCache != nil)
		{
			status_t	err1 = fFileCache->GetFile(outReply.handle, inQuery.fileName, inQuery.inSysTree);
			if (err1 == B_NO_ERROR)
				fLastFile = outReply.handle;
		}
	}
	else
		err = B_FILE_NOT_FOUND;

	return err;
}

// ---------------------------------------------------------------------------

status_t
MCompilerObj::DoMessage(const ErrorNotificationMessage& message)
{
	// An error or warning message has been received from the compiler.  We package
	// it in a BMessage and send it to the message window.

	fProjectView.GetErrorMessageWindow()->PostErrorMessage(message);

	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------

status_t
MCompilerObj::DoMessage(const ErrorNotificationMessageShort& message)
{
	fProjectView.GetErrorMessageWindow()->PostErrorMessage(message);

	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		DoStatus
// ---------------------------------------------------------------------------
// Called after each source file line.

void
MCompilerObj::DoStatus(
	const CompilerStatusNotification& inRec)
{
	if (fSourceFileLine)
	{
		// Send the browse data to the sourcefileline
		if (inRec.browseErrorCode == B_NO_ERROR && fAreaID >= B_NO_ERROR)
		{
			area_info			info;
			
			if (B_NO_ERROR == get_area_info(fAreaID, &info))
			{
				BrowseDataArea*		browseArea = (BrowseDataArea*) info.address;
				char*				browseData = nil;
				if (browseArea->length > 0 )
				{
					browseData = new char[browseArea->length];
					memcpy(browseData, browseArea->data, browseArea->length);
				}

				fSourceFileLine->SetBrowseData(browseData, browseArea->length);
			}
		}

		// Tell it that it's done
		// Notice that fHeaderFileList is only set up by IDEAware tools
		// Non IDEAware tools will set up the dependency information by a call to
		// GenerateDependencies
		fSourceFileLine->CompileDone(inRec.errorCode,
									 inRec.codeSize,
									 inRec.dataSize,
									 this->IsIDEAware() ? &fHeaderFileList : NULL);
		fSourceFileCalled = true;
	}
}

// ---------------------------------------------------------------------------
//		DoStatus
// ---------------------------------------------------------------------------
//	Called after all source file lines have been compiled.

void
MCompilerObj::DoStatus(
	bool		objProduced,
	status_t	errCode)
{
	if (fSourceFileLine == nil)
		fProjectView.CompileDone();
	else
	if (!fSourceFileCalled)
		fSourceFileLine->CompileDone(errCode, 0, 0, nil);

	MCompileTool::DoStatus(objProduced, errCode);
}

// ---------------------------------------------------------------------------
//		DoPreprocesResult
// ---------------------------------------------------------------------------
//	Handle a response to a preprocess file request.  Clone the area containing
//	the text and post a message to the app to open the text window.  We wait
//	til the app is finished and then delete the area.

void
MCompilerObj::DoPreprocesResult(
	const SendTextMessage& textMessage)
{
	void*		areaAddress = (void*) B_PAGE_SIZE;
	area_id		clone = clone_area("idearea", &areaAddress, B_ANY_ADDRESS, B_READ_AREA, textMessage.textArea);

	if (clone >= B_NO_ERROR)
	{
		int32			size = textMessage.textSize - 1;
		const char*		text = (const char*) areaAddress;
		
		while (size > 0 && text[size - 1] == 0)		// usually a null at the end
		{
			size--;
		}

		// need to get the name for files that aren't in the project
		String		name;
		
		if (fSourceFileLine)
			name = fSourceFileLine->GetFileName();
		else
			name = fFileName;

		switch (fCompileKind)
		{
			case kPreprocess:
				name.Insert("#", 0);
				break;

			case kDisassemble:
				name += ".dump";
				break;
		}
			
		// Send a syncronous message so we don't return to the compiler
		// before we're done with the text in the area
		BMessenger		app(be_app);
		BMessage		reply;
		BMessage		msg(msgRawTextWindow);
		
		msg.AddPointer(kAddress, text);
		msg.AddInt32(kSize, size);
		msg.AddString(kFileName, name);

		app.SendMessage(&msg, &reply);

		delete_area(clone);
	}
}

// ---------------------------------------------------------------------------
//		GetArea
// ---------------------------------------------------------------------------

void
MCompilerObj::GetArea(
	GetAreaReply& areaMessage)
{
	areaMessage.area = fAreaID;
}

// ---------------------------------------------------------------------------
//		ParseMessageText
// ---------------------------------------------------------------------------
//	return B_ERROR if there are errors in the text and
//	B_NO_ERROR if there are only warnings.

status_t
MCompilerObj::ParseMessageText(
	const char*	inText)
{
	status_t	err = B_NO_ERROR;
	BList		messageList;

	if (fSourceFileLine != nil)
	{
		fSourceFileLine->ParseMessageText(inText, messageList);
	}
	
	if (messageList.CountItems() > 0)
	{
		bool			hasErrors = false;
		BMessage		bmsg(msgAddErrorsToMessageWindow);

		for (int32 i = 0; i < messageList.CountItems(); i++)
		{
			ErrorMessage*	msg = (ErrorMessage*) messageList.ItemAt(i);
			ASSERT(msg != nil);

			if (msg != nil)
			{
				hasErrors = hasErrors | ! msg->isWarning;
				msg->filename[MAX_ERROR_PATH_LENGTH-1] = '\0';		// protect against bad strings from the add_ons
				msg->errorMessage[MAX_ERROR_TEXT_LENGTH-1] = '\0';
				bmsg.AddData(kPlugInErrorName, kErrorType, msg, sizeof(ErrorMessage));
				delete msg;
			}
		}
		
		fProjectView.GetErrorMessageWindow()->PostMessage(&bmsg);
		
		if (hasErrors) {
			err = B_ERROR;
		}
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		CodeDataSize
// ---------------------------------------------------------------------------

void
MCompilerObj::CodeDataSize(
	int32&	outCodeSize,
	int32&	outDataSize)
{
	if (fSourceFileLine != nil)
		fSourceFileLine->CodeDataSize(outCodeSize, outDataSize);
}

// ---------------------------------------------------------------------------
//		GenerateDependencies
// ---------------------------------------------------------------------------

void
MCompilerObj::GenerateDependencies()
{
	if (fSourceFileLine != nil)
		fSourceFileLine->GenerateDependencies();
}
