//========================================================================
//	BResBuilder.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Jon Watte

#include <string.h>
#include <ctype.h>
#include <alloca.h>
#include <ByteOrder.h>
#include <Entry.h>
#include <Path.h>

#include "BResBuilder.h"
#include "BResPlugInView.h"
#include "PlugInPreferences.h"
#include "ErrorMessage.h"
#include "MProject.h"

const char *	mwbresToolName = "mwbres";

#if __INTEL__
// needed on intel for some reason
//char * alloca(int);
#endif

// ---------------------------------------------------------------------------
//		GetToolName
// ---------------------------------------------------------------------------

long
BResBuilder::GetToolName(
	MProject*	/*inProject*/,
	char* 		outName,
	long		/*inBufferLength*/,
	MakeStageT	/*inStage*/,
	MakeActionT	/*inAction*/)
{
	strcpy(outName, mwbresToolName);

	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		LinkerName
// ---------------------------------------------------------------------------

const char *
BResBuilder::LinkerName()
{
	return B_EMPTY_STRING;	// not associated with any linker
}

// ---------------------------------------------------------------------------
//		MakeStages
// ---------------------------------------------------------------------------

MakeStageT
BResBuilder::MakeStages()
{
	return (kPostLinkStage);
}

// ---------------------------------------------------------------------------
//		Actions
// ---------------------------------------------------------------------------

MakeActionT
BResBuilder::Actions()
{
	return (kPostLinkExecute);
}

// ---------------------------------------------------------------------------
//		Flags
// ---------------------------------------------------------------------------

PlugInFlagsT
BResBuilder::Flags()
{
	return kNotIDEAware;
}

// ---------------------------------------------------------------------------
//		MessageDataType
// ---------------------------------------------------------------------------

ulong
BResBuilder::MessageDataType()
{
	return kResMessageType;
}

// ---------------------------------------------------------------------------
//		BuildPrecompileArgv
// ---------------------------------------------------------------------------

long
BResBuilder::BuildPrecompileArgv(
	MProject&	/* inProject */,
	BList& 		inArgv,
	MFileRec& 	inFileRec)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		BuildCompileArgv
// ---------------------------------------------------------------------------

long
BResBuilder::BuildCompileArgv(
	MProject&	/* inProject */,
	BList& 		inArgv,
	MakeActionT /*inAction*/,
	MFileRec& 	inFileRec)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		BuildPostLinkArgv
// ---------------------------------------------------------------------------

long
BResBuilder::BuildPostLinkArgv(
	MProject&	inProject,
	BList& 		inArgv,
	MFileRec& 	inFileRec)
{
	entry_ref		ref;
	status_t		err = inProject.GetExecutableRef(ref);
	if (err) return err;
	BEntry entry(&ref);
	err = entry.InitCheck();
	if (err) return err;
	BPath	path;
	err = entry.GetPath(&path);
	if (err) return err;
	inArgv.AddItem(strdup("-merge"));
	inArgv.AddItem(strdup("-o"));
	inArgv.AddItem(strdup(path.Path()));
	inArgv.AddItem(strdup(inFileRec.path));
	return B_NO_ERROR;
}


// ---------------------------------------------------------------------------
//		ValidateSettings
// ---------------------------------------------------------------------------
//	Validate the settings in the message.  If they are not valid for this
//	version of the plug-in or they don't exist then a valid prefs struct is
//	added to the message.

bool
BResBuilder::ValidateSettings(BMessage&	inOutMessage)
{
	ResPrefs	defaultPrefs = { kCurrentVersion };
	bool		changed = false;
	ResPrefs*	prefsPtr;
	long		len;

	if (B_OK == inOutMessage.FindData(kResMessageName, kResMessageType, (const void**) &prefsPtr, &len))
	{
		if (B_BENDIAN_TO_HOST_INT32(prefsPtr->version) != kCurrentVersion || len != sizeof(ResPrefs))
		{
			defaultPrefs.SwapHostToBig();
			inOutMessage.ReplaceData(kResMessageName, kResMessageType, &defaultPrefs, sizeof(defaultPrefs));
			changed = true;
		}
	}
	else
	{
		defaultPrefs.SwapHostToBig();
		inOutMessage.AddData(kResMessageName, kResMessageType, &defaultPrefs, sizeof(defaultPrefs));
		changed = true;
	}
	
	return changed;
}

// ---------------------------------------------------------------------------
//		FindEndOfLine
// ---------------------------------------------------------------------------

long
BResBuilder::FindEndOfLine(
	const char*	inText,
	long		inOffset)
{
	while (inText[inOffset] != '\n' && inText[inOffset] != 0)
		inOffset++;

	return inOffset;
}

// ---------------------------------------------------------------------------
//		ParseError
// ---------------------------------------------------------------------------
//	Given a certain chunk of text, parse out the file, line and offset 
//	information and return an appropriate error message struct.

ErrorMessage*
BResBuilder::ParseError(
	const char*	inBeg,
	const char*	inEnd)
{
	const char *pos = inBeg;

	if (strncmp(inBeg, "File ", 5))
		goto textOnly;
	// scoped so that gcc doesn't complain about jump to textOnly
	// crossing initializations made below
	{
		pos += 5;
		const char *base = pos;
		while (pos < inEnd)
			if (*pos == ';')
				break;
			else
				pos++;
		if (base == pos)
			goto textOnly;
		char *name = (char *)alloca(pos-base+1);
		strncpy(name, base, pos-base);
		name[pos-base] = 0;
	
		if (strncmp(pos, "; Line ", 7))
			goto textOnly;
		pos += 7;
		long line = 0;
		while (isdigit(*pos) && (pos<inEnd)) {
			line *= 10;
			line += (*pos)-'0';
			pos++;
		}
		if (!line)
			goto textOnly;
	
		if (strncmp(pos, "; Offset ", 9))
			goto textOnly;
		pos += 9;
		long offset = 0;
		while (isdigit(*pos) && (pos<inEnd)) {
			offset *= 10;
			offset += (*pos)-'0';
			pos++;
		}
		while (*pos != '\n')
			pos++;
		pos++;
		while (inEnd[-1] == '\n')
			inEnd--;
	
		{
			//	so we know we have OK data for the ErrorRef
			ErrorMessage *ret = new ErrorMessage;
			ret->linenumber = line;
			ret->offset = offset;
			ret->length = 0;
			ret->sync[0] = 0;
			ret->synclen = 0;
			ret->syncoffset = offset;
			ret->errorlength = 0;
			ret->erroroffset = 0;
			ret->textonly = false;
			ret->isWarning = false;
	//		strcpy(ret->filename, "\\");
	//		strcat(ret->filename, name);
			strcpy(ret->filename, name);
			int msglen = inEnd-pos;
			if (msglen > MAX_ERROR_TEXT_LENGTH-1)
				msglen = MAX_ERROR_TEXT_LENGTH-1;
			strncpy(ret->errorMessage, pos, msglen);
			ret->errorMessage[msglen] = 0;
	
			return ret;
		}
	}
	
textOnly:
	{
		//	we don't know what file or line
		ErrorMessage *ret = new ErrorMessage;
		ret->linenumber = 0;
		ret->offset = 0;
		ret->length = 0;
		ret->sync[0] = 0;
		ret->synclen = 0;
		ret->syncoffset = 0;
		ret->errorlength = 0;
		ret->erroroffset = 0;
		ret->textonly = true;
		ret->isWarning = false;
		ret->filename[0] = 0;
		int msglen = inEnd-inBeg;
		if (msglen > MAX_ERROR_TEXT_LENGTH-1)
			msglen = MAX_ERROR_TEXT_LENGTH-1;
		strncpy(ret->errorMessage, inBeg, msglen);
		ret->errorMessage[msglen] = 0;

		return ret;
	}
}


// ---------------------------------------------------------------------------
//		ParseMessageText
// ---------------------------------------------------------------------------
//	Read the message text and break it into ErrorMessages.  Add each
//	ErrorMessage to the BList.  The text is redirected output of stderr and
//	stdout.  The text is guaranteed to be null terminated.

long
BResBuilder::ParseMessageText(
	MProject&	/* inProject */,
	const char*	inText,
	BList&		outList)
{
	const char *pos = inText;
	long len = strlen(inText);
	const char *end = pos+len;
	const char *errbeg = NULL;
	int onLine = 1;
	while (pos < end) {
		if (onLine) {
			if (!strncmp(pos, "File ", 5)) {
				if (errbeg) {
					ErrorMessage *msg = ParseError(errbeg, pos);
					if (msg) outList.AddItem(msg);
				}
				errbeg = pos;
			}
			onLine = 0;
		}
		if (*pos == '\n') {
			onLine = 1;
		}
		pos++;
	}
	if (errbeg && (errbeg<pos)) {
		ErrorMessage *msg = ParseError(errbeg, pos);
		if (msg) outList.AddItem(msg);
	}

	// [note by John Dance]
	// Why is this new ErrorMessage here?  
	// I assumed it has no good cause and commented it out.
	// ErrorMessage*	msg = new ErrorMessage;
	
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		CodeDataSize
// ---------------------------------------------------------------------------

void
BResBuilder::CodeDataSize(
	MProject&	/*inProject*/,
	const char* /*inFilePath*/,
	long&	outCodeSize,
	long&	outDataSize)
{
	outCodeSize = -1;
	outDataSize = -1;
}

// ---------------------------------------------------------------------------
//		GenerateDependencies
// ---------------------------------------------------------------------------

long
BResBuilder::GenerateDependencies(
	MProject&	/*inProject*/,
	const char*	/*inFilePath*/,
	BList&		/*outList*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		FileIsDirty
// ---------------------------------------------------------------------------
//	return true if the file is dirty and needs to be executed as part of a 
//	make or bring up to date.
//	We always execute mwbres scripts so always return true.

bool
BResBuilder::FileIsDirty(
	MProject&	/*inProject*/,
	MFileRec& 	/*inFileRec*/,
	MakeStageT	/*inStage*/,
	MakeActionT	/*inAction*/,
	time_t		/*inModDate*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		GetTargetFilePaths
// ---------------------------------------------------------------------------

void
BResBuilder::GetTargetFilePaths(
	MProject&		/* inProject */,
	MFileRec& 		/*inFileRec*/,
	BList&			/*inOutTargetFileList*/)
{
	// Is this 'inFileName.out' ?
}

// ---------------------------------------------------------------------------
//		ProjectChanged
// ---------------------------------------------------------------------------

void
BResBuilder::ProjectChanged(
	MProject&	/*inProject*/,
	ChangeT		inChange)
{
	switch (inChange)
	{
		case kBuildStarted:
		case kProjectOpened:
		case kProjectClosed:
		case kFilesAdded:
		case kFilesRemoved:
		case kFilesRearranged:
		case kPrefsChanged:
		case kRunMenuItemChanged:
		case kLinkDone:
			// Don't do anything for now
			break;
	}
}


