//========================================================================
//	MFindThing.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>
#include <ctype.h>

#include "MFindThing.h"
#include "MFindWindow.h"
#include "MLookupThreadInformationWindow.h"
#include "MTextWindow.h"
#include "MIDETextView.h"
#include "MMultiFileListView.h"
#include "MMultiFindThread.h"
#include "MSourceFileList.h"
#include "MFormatUtils.h"
#include "MSourceFile.h"
#include "MRegExpErrors.h"
#include "MWMunger.h"
#include "MWRegExp.h"
#include "MWEditUtils.h"
#include "MAlert.h"
#include "IDEApp.h"
#include "IDEMessages.h"
#include "Utils.h"

#include <String.h>
#include <Beep.h>

short	MFindThing::fRegExpError;

// ---------------------------------------------------------------------------
//		MFindThing
// ---------------------------------------------------------------------------
//	Constructor

MFindThing::MFindThing()
{
	fMultiFileThread = nil;
	fListView = nil;
	fAllList = nil;
	fFindInNextFile = false;
}

// ---------------------------------------------------------------------------
//		~MFindThing
// ---------------------------------------------------------------------------
//	Denstructor

MFindThing::~MFindThing()
{
	CancelMultiFindThread();
}

// ---------------------------------------------------------------------------
//		CancelMultiFindThread
// ---------------------------------------------------------------------------

void
MFindThing::CancelMultiFindThread()
{
	// MFindWindow doesn't know if there is an active thread or not
	if (fMultiFileThread) {
		fMultiFileThread->Cancel();
		fMultiFileThread = nil;
	}
}

// ---------------------------------------------------------------------------
//		CanFind
// ---------------------------------------------------------------------------

bool
MFindThing::CanFind()
{
	return fMultiFileThread == nil;
}

// ---------------------------------------------------------------------------
//		DoFindNext
// ---------------------------------------------------------------------------
//	Called from a text window.

bool
MFindThing::DoFindNext(
	MIDETextView& 	inTextView,
	bool			inBeepIfNotFound)
{
	int32		selStart;
	int32		selEnd;
	int32		foundOffset;
	int32		findStringLength;
	bool		found;

	inTextView.GetSelection(&selStart, &selEnd);

	int32		startOffset = selEnd;
	if (! fData.fForward)
		startOffset = selStart;

	found = FindNext(inTextView.Text(), inTextView.TextLength(), 
					startOffset, foundOffset, findStringLength, fData.fWrap);
	if (found)
	{
		inTextView.Select(foundOffset, foundOffset + findStringLength);
		inTextView.ScrollToSelection();
	}
	else
		if (inBeepIfNotFound)
			beep();

	return found;
}

// ---------------------------------------------------------------------------
//		DoReplace
// ---------------------------------------------------------------------------

bool
MFindThing::DoReplace(
	MIDETextView& 	inTextView)
{
	// Check that the selection hasn't changed
	bool		result = ConfirmSelectionValid(inTextView) && inTextView.IsEditable();

	if (result)
	{
		// Replace the text
		const char *		replaceString = fData.ReplaceString;
		
		if (!fData.fRegexp)
		{
			inTextView.Replace(replaceString);
		}
		else
		{
			char*			newText = REReplace(replaceString);
			
			inTextView.Replace(newText);
			
			delete[] newText;
		}

		inTextView.SetDirty();
	}
	else
		beep();
		
	return result;
}

// ---------------------------------------------------------------------------
//		DoReplaceAndFind
// ---------------------------------------------------------------------------

void
MFindThing::DoReplaceAndFind(
	MIDETextView& 	inTextView)
{

	if (DoReplace(inTextView))
	{
		if (! fData.fMultiFile)
			DoFindNext(inTextView, false);
		else
			DoMultiFileFind();
	}
}

// ---------------------------------------------------------------------------
//		DoReplaceAll
// ---------------------------------------------------------------------------

void
MFindThing::DoReplaceAll(
	MIDETextView& 	inTextView,
	bool			inBeepIfNotFound)
{
	if (inTextView.IsEditable())
	{
		bool			wrap = fData.fWrap;
		fData.fWrap = false;
	
		inTextView.Select(0, 0);
		
		while (DoFindNext(inTextView, inBeepIfNotFound))
			DoReplace(inTextView);
	
		fData.fWrap = wrap;
	}
	else
	if (inBeepIfNotFound)
		beep();
}

// ---------------------------------------------------------------------------
//		DoFindInNextFile
// ---------------------------------------------------------------------------

void
MFindThing::DoFindInNextFile()
{
	IncrementMultiIndex();
	fFindInNextFile = true;
	DoMultiFileFind();
}

// ---------------------------------------------------------------------------
//		DoBatchFind
// ---------------------------------------------------------------------------
//	Do a batch find for a single file.  Window is locked by the caller.

void
MFindThing::DoBatchFind(
	MTextWindow&	inTextWindow)
{
	MIDETextView*	TextView = inTextWindow.GetTextView();
	const char * 	text = TextView->Text();
	int32			textLength = TextView->TextLength();
	entry_ref		ref;
	status_t		err = inTextWindow.GetRef(ref);
	ASSERT(err == B_NO_ERROR);
	bool			foundSomething = false;

	// make sure we start with a fresh message window
	fMessageWindow = nil;

	foundSomething = BatchFind(text, textLength, ref, inTextWindow.Title(), kDontWrap);

	if (foundSomething) {
		this->GetMessageWindow()->PostMessage(msgShowAndActivate);
		this->GetMessageWindow()->PostMessage(msgDoneWithMessageWindow);
	}
	else
		beep();
}

// ---------------------------------------------------------------------------
//		BatchFind
// ---------------------------------------------------------------------------
//	Do a batch find for a given text pointer and length.  return true if
//	something was found.

bool
MFindThing::BatchFind(
	const char*		inText,
	int32			inLength,
	entry_ref&		inRef,
	const char *	inFileName,
	bool			inWrap)
{
	bool			foundSomething = false;
	int32			findStringLength;
	int32			foundOffset = 0;
	int32			line = 0;
	int32			lineStart = 0;
	bool			found = true;
	String			msgText;
	const char *	text = inText;
	int32			textLength = inLength;
	BMessage		msg(msgAddInfoToMessageWindow);
	int32			itemCount = 0;
	const int32		kItemsPerMessage = 20;

	while (found)
	{
		found = FindNext(text, inLength, foundOffset, foundOffset, findStringLength, inWrap);

		if (found)
		{
			// Build the info struct
			InfoStruct	 	info;
	
			line = LineOf(text, foundOffset, line, lineStart);

			info.iTextOnly = false;
			info.iRef = new entry_ref(inRef);
			info.iLineNumber = line;		// message item lines count from 0
			strcpy(info.iFileName, inFileName);
			
			int32		lineLength = min(LineEndOffset(text, lineStart, textLength) - lineStart, 255L);

			memcpy(info.iLineText, text + lineStart, lineLength);
			info.iLineText[lineLength] = 0;

			// Build the token id struct
			info.iToken.eLineNumber = line;
			info.iToken.eOffset = foundOffset;
			info.iToken.eLength = findStringLength;
			info.iToken.eSyncLength = min(31L, findStringLength);
			info.iToken.eSyncOffset = 0;
			memcpy(info.iToken.eSync, &text[foundOffset], info.iToken.eSyncLength);
			info.iToken.eSync[info.iToken.eSyncLength] = 0;
			info.iToken.eIsFunction = false;

			msg.AddData(kInfoStruct, kInfoType, &info, sizeof(info), false, kItemsPerMessage);
			
			if (++itemCount == kItemsPerMessage)
			{
				// Post the message
				this->GetMessageWindow()->PostMessage(&msg);
				msg.MakeEmpty();
				itemCount = 0;
			}

			foundOffset += findStringLength;
			foundSomething = true;
		}		
	}

	if (itemCount > 0 ) {
		this->GetMessageWindow()->PostMessage(&msg);
	}
	
	return foundSomething;
}

// ---------------------------------------------------------------------------
//		LineEndOffset
// ---------------------------------------------------------------------------

int32
MFindThing::LineEndOffset(
	const char*		inText,
	int32			inOffset,
	int32			inTextLen)
{
	return FindEnd(inText + inOffset, inTextLen - inOffset) - inText;
}

// ---------------------------------------------------------------------------
//		LineOf
// ---------------------------------------------------------------------------
//	Get the linenumber of the specified offset.  The caller is expected to
//	cache the oldLineNumber and matching offset of its linestart.

int32
MFindThing::LineOf(
	const char* 	inText,
	int32			inOffset,
	int32			inOldLineNumber,
	int32&			inOutOldOffset)
{
	int32			currentLine = inOldLineNumber;
	int32			currentLineOffset = inOutOldOffset;
	const char *	text = inText;
	
	for (int32 i = inOutOldOffset; i < inOffset; i++)
	{
		if (text[i] == '\n')
		{
			currentLine++;
			currentLineOffset = i + 1;
		}
	}
	
	inOutOldOffset = currentLineOffset;

	return currentLine;
}

// ---------------------------------------------------------------------------
//		FindNext
// ---------------------------------------------------------------------------
//	Find the findstring in the text specified.  If found, returns true and sets
//	the outOffset and outLength. 

bool
MFindThing::FindNext(
	const char* inText, 
	int32 		inTextLength, 
	int32 		inOffset, 
	int32& 		outOffset, 
	int32& 		outLength,
	bool		inWrap)
{
	int32			start;
	int32			end;
	int32			timesDone = 0;
	bool			found = false;
	int32			offset = inOffset;
	int32			textLength = inTextLength;
	String			findString = fData.FindString;
	int32			findStringLength =  findString.GetLength();
	
	while ((! inWrap && timesDone == 0) || (inWrap && timesDone < 2 && ! found))
	{
		if (timesDone == 1)
		{
			if (fData.fForward)
			{
				offset = 0;
				textLength = inOffset;
			}
			else
			{
				offset = inTextLength;
			}
		}

		if (!fData.fRegexp)
		{
			start = MWMunger((char *) (const char*) findString, findStringLength, (char *) inText,
							 textLength, offset, fData.fForward, fData.fEntireWord,
							 ! fData.fIgnoreCase);
		
			if (start != -1)
			{
				found = true;
				outLength = findStringLength;
				outOffset = start;
			}
		}
		else
		{
			fRegExpError = no_error;	// Reset the Regexp error global

			if (REPrep(!fData.fIgnoreCase, false, (char*) (const char*) findString, (REErrorProc) HandleRegExpError) &&
				DoRESearch(! fData.fForward, fData.fEntireWord, (char*) inText, offset, textLength, &start, &end)) 
			{
				outOffset = start;
				outLength = end - start;

				found = true;
			}
		}
		
		timesDone++;
	}
	
	return found;
}

// ---------------------------------------------------------------------------
//		HandleRegExpError
// ---------------------------------------------------------------------------

void
MFindThing::HandleRegExpError(
	short	inCode)
{
	fRegExpError = inCode;
	String		text;
	
	switch (inCode)
	{
		case no_error:
			break;
		
		case invalid_args:
		case invalid_pgm:
		case corrupted_mem:
		case corrupted_ptrs:
		case internal_error:
		case corrupted_opcode:
		case damaged_match_str:
		case junk_on_end:
		case internal_urp:		/*	"internal urp": should never happen */
		case internal_disaster:	/*	"internal disaster" */
			text = FindErrInternal;
			break;
		case expr_too_complicated:
			text = FindErrTooComplicated;
			break;
		case out_of_memory:
			text = FindErrNoMem;
			break;
		case too_many_subexpr:
			text = FindErrTooManySubs;
			break;
		case unmatched_parens:
			text = FindErrUnmatchedParenthesis;
			break;
		case empty_star_plus:		/* "*+ operand could be empty" */
			text = FindErrEmptyStarPlus;
			break;
		case nested_repeater:		/* "nested *?+" */
			text = FindErrTooComplicated;
			break;
		case invalid_range:			/* "invalid [] range" */
			text = FindErrInvalidRange;
			break;
		case unmatched_bracket:
			text = FindErrUnmatchedBracket;
			break;
		case bad_repeater:			/*	"?+* follows nothing" */
			text = FindErrBadRepeater;
			break;
		case trailing_backslash:	/*	"trailing \\" */
			text = FindErrTrailingBackslash;
			break;
		case too_much_recursion:	/* GRS: Prevents stack/heap collision */
			text = FindErrTooMuchRecursion;
			break;
	}
	
	// Show the alert
	if (inCode != no_error)
	{
		MAlert		alert(text);
		
		alert.Go();
	}
}

// ---------------------------------------------------------------------------
//		ConfirmSelectionValid
// ---------------------------------------------------------------------------
//	Need to confirm that the selection matches the find string when doing a
//	replace since it could have been changed by the user.

bool
MFindThing::ConfirmSelectionValid(
	MIDETextView& 	inTextView)
{
	int32			selStart;
	int32			selEnd;
	bool	 		result;
	
	inTextView.GetSelection(&selStart, &selEnd);

	result = ConfirmSelectionValid(inTextView.Text(), selStart, selEnd);

	return result;
}

// ---------------------------------------------------------------------------
//		IsWhiteSpace
// ---------------------------------------------------------------------------

inline bool
MFindThing::IsWhiteSpace(
	char 	inChar)
{
	return !(isalpha(inChar) || inChar == '_' || (inChar >= '0' && inChar <= '9'));
}

// ---------------------------------------------------------------------------
//		ConfirmSelectionValid
// ---------------------------------------------------------------------------
//	Need to confirm that the selection matches the find string when doing a
//	replace since it could have been changed by the user.

bool
MFindThing::ConfirmSelectionValid(
	const char*		inText,
	int32			inSelStart,
	int32			inSelEnd)
{
	bool	 		result = false;
	String			findString = fData.FindString;

	if (! fData.fRegexp)
	{
		if (findString.GetLength() == inSelEnd - inSelStart &&
			EqualText(findString, inText + inSelStart, fData.fIgnoreCase) &&
			(!fData.fEntireWord || IsWhiteSpace(inText[inSelEnd])))
				result = true;
	}
	else
	{
		// RegExp case
		// Reset the Regexp error global
		fRegExpError = no_error;

		int32 			start;
		int32 			end;
		
		if (REPrep(!fData.fIgnoreCase, false, (char*) (const char*) findString, (REErrorProc) HandleRegExpError) &&
			DoRESearch(false, fData.fEntireWord, (char*) (const char*) inText, inSelStart, inSelEnd, &start, &end)) 
		{
			result = (inSelStart == start) && (inSelEnd == end);
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		EqualText
// ---------------------------------------------------------------------------
//	Compare a string and raw text.

bool
MFindThing::EqualText(
	const char* 	inString,			// null terminated string
	const char* 	inText, 			// raw text
	bool 			inCaseInsensitive)
{
	bool			result = true;

	if (inCaseInsensitive)
	{
		// Case insensitive bytewise comparison
		const char *	toupp = (char *)__CUppr;

		while (*inString) 
		{
			if (toupp[*inString] != toupp[*inText])
			{
				result = false;
				break;
			}
			
			inString++; 
			inText++;
		}
	}
	else
	{
		// Case sensitive bytewise comparison
		while (*inString) 
		{
			if ((*inString) != (*inText))
			{
				result = false;
				break;
			}
			
			inString++; 
			inText++;
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		DoMultifileFind
// ---------------------------------------------------------------------------
//	Start a MultiFileFind.  note default parameter.

void
MFindThing::DoMultiFileFind()
{
	if (fData.fBatch) {
		// make sure we start with a fresh message window
		fMessageWindow = nil;
	}

	fData.fOnlyOne = fData.fStopAtEOF && ! fData.fBatch && ! fData.fResetMulti 
					&& ! fFindInNextFile;
	
	fStartMultiIndex = fCurrentMultiIndex;

	fMultiFileThread = new MMultiFindThread(*this);
	if (B_NO_ERROR != fMultiFileThread->Run())
	{
		ASSERT(!"Couldn't start multifilefind thread");
		delete fMultiFileThread;
		fMultiFileThread = nil;
	}
}

// ---------------------------------------------------------------------------
//		ExecuteMultiFind
// ---------------------------------------------------------------------------
//	Called from the multifilefind thread.

bool
MFindThing::ExecuteMultiFind(
	MMultiFindThread&	inFindThread)
{
	bool			more = true;
	bool			found = false;
	bool			foundSomething = false;
	sem_id			fileOpened = 0;
	bool			error = false;
	bool			beginNewFile = fData.fResetMulti || fFindInNextFile;
	String			fileName;
	BFile			file;
	char*			textBuffer = nil;
	int32			textBufferSize;
	status_t		err;
	const int32		kMinTextBufferSize = (1024 * 16) - 1;// 16K minimum size for buffer

	while (more && ! inFindThread.Cancelled())
	{
		MSourceFile*		sourceFile = nil;
		entry_ref			ref;

		fAllList->GetNthItem(sourceFile, fCurrentMultiIndex);

		if (sourceFile != nil && B_NO_ERROR == sourceFile->GetRef(ref))
		{
			// Update the position of the Blue Arrow
			MFindWindow::GetFindWindow().Lock();
			fListView->SetBlueRow(fCurrentMultiIndex);
			fListView->Sync();
			MFindWindow::GetFindWindow().Unlock();
		}

		if (ref.device != -1)
		{
			const char* text = nil;
			off_t textLength = 0;
			int32 foundOffset = 0;
			int32 findStringLength = 0;
			MTextWindow* wind = MDynamicMenuHandler::FindWindow(ref);
			MIDETextView* textView = nil;

			fileName = sourceFile->GetFileName();

			// Get a pointer and length to the text either from an open
			// window or by reading in the file

			// Is the window for this file open?
			if (wind && wind->Lock())
			{
				textView = wind->GetTextView();
				text = textView->Text();
				textLength = textView->TextLength();
			}
			else
			{
				// If not, open the file and read in the text
				err = file.SetTo(&ref, B_READ_ONLY);
				err = file.GetSize(&textLength);
				if (textBuffer == nil || textLength > textBufferSize)
				{
					delete[] textBuffer;
					textBufferSize = max((int32) textLength, kMinTextBufferSize);
					textBuffer = new char[textBufferSize + 1];
				}

				text = textBuffer;

				if (err == B_NO_ERROR) {
					err = file.Read(textBuffer, textLength) >= 0 ? B_NO_ERROR : err;
				}
				textBuffer[textLength] = 0;

				// Deal with returns or cr-lfs here
				TextFormatType format = MFormatUtils::FindFileFormat(textBuffer);
				
				if (format != kNewLineFormat) {
					MFormatUtils::ConvertToNewLineFormat(textBuffer, textLength, format);
				}
			}

			// Do the find
			if (fData.fBatch)
			{
				found = BatchFind(text, textLength, ref, fileName, kDontWrap);
			}
			else
			if (fData.fReplaceAll)
			{
				if (wind)
					DoReplaceAll(*textView, false);
				else
				{
					found = FindNext(text, textLength, 0, foundOffset, findStringLength, kDontWrap);
					
					if (found)
					{
						error = OpenAndReplaceAll(fileOpened, ref);
						found = false;
					}
				}
			}
			else
			{
				if (wind)
				{
					int32		selStart;
					int32		startOffset;
					
					// Is this the first time we are searching this window in this loop?
					if (beginNewFile)
					{
						if (fData.fForward)
							startOffset = 0;
						else
							startOffset = textLength - 1;
					}
					else
					{
						textView->GetSelection(&selStart, &startOffset);
						if (! fData.fForward)
							startOffset = selStart;
					}

					found = FindNext(text, textLength, startOffset, foundOffset, findStringLength, kDontWrap);
									
					if (found)
					{
						textView->Select(foundOffset, foundOffset + findStringLength);
						textView->ScrollToSelection();
						wind->Activate();
					}
				}
				else
				{
					int32		startOffset = 0;
					if (! fData.fForward)
						startOffset = textLength - 1;
					
					found = FindNext(text, textLength, startOffset, foundOffset, findStringLength, kDontWrap);
					
					// If not a batch find then we need to tell the app to open the window
					if (found)
					{
						TokenIdentifier		token;
						int32				line = 0;
						int32				lineStart = 0;
						
						token.eLineNumber = LineOf(text, foundOffset, line, lineStart);
						token.eOffset = foundOffset;
						token.eLength = findStringLength;
						token.eSyncLength = min(31L, findStringLength);
						token.eSyncOffset = 0;
						memcpy(token.eSync, &text[foundOffset], token.eSyncLength);
						token.eSync[token.eSyncLength] = 0;
						token.eIsFunction = false;

						BMessage 			msg(msgOpenSourceFile);
						msg.AddRef("refs", &ref);
						msg.AddData(kTokenIdentifier, kTokenIDType, &token, sizeof(token));
						be_app_messenger.SendMessage(&msg);
					}
				}
			}

			if (wind)
				wind->Unlock();
		}
		
		// Increment counter
		if (! fData.fOnlyOne && (fData.fBatch || ! found))
		{
			IncrementMultiIndex();
			beginNewFile = true;
		}
		else
			beginNewFile = false;

		// Do we go again?
		if (((! fData.fBatch && found) && ! fData.fReplaceAll) || 
			((fData.fForward && fCurrentMultiIndex >= fAllList->CountItems()) || 
				(!fData.fForward && fCurrentMultiIndex < 0)) ||
			(fCurrentMultiIndex == fStartMultiIndex) ||
			(fData.fOnlyOne) ||
			(fRegExpError != no_error) ||
			(error) )
		{
			more = false;
		}
	
		foundSomething = foundSomething || found;
	}

	delete[] textBuffer;

	if (fileOpened > B_NO_ERROR)
		delete_sem(fileOpened);

	return foundSomething;
}

// ---------------------------------------------------------------------------
//		OpenAndReplaceAll
// ---------------------------------------------------------------------------
//	Ask the app to open the file.  Wait for it to complete and then
//	ReplaceAll.  Calling code can create the sem once and pass it
//	repeatedly to this function.

bool
MFindThing::OpenAndReplaceAll(
	sem_id&				inSem,
	const entry_ref&	inRef)
{
	bool		error = false;

	// Use a semaphore so we know when the window has been opened
	// when replacing
	if (inSem == 0)
	{
		inSem = create_sem(0, "OpenFile");		// sem count is zero
		if (inSem < B_NO_ERROR)
			error = true;
	}

	// Build and send the message
	BMessage 			msg(msgOpenSourceFile);
	msg.AddRef("refs", &inRef);
	if (inSem > B_NO_ERROR)
		msg.AddInt32(kSemID, inSem);
	be_app_messenger.SendMessage(&msg);

	// Wait for the app to open the window before replacing
	if (inSem > B_NO_ERROR)
	{
		acquire_sem(inSem);

		MTextWindow*		wind = MDynamicMenuHandler::FindWindow(inRef);
		if (wind && wind->Lock())
		{
			MIDETextView*		textView = wind->GetTextView();
			DoReplaceAll(*textView, false);
			wind->Unlock();
		}
	}
	
	return error;
}

// ---------------------------------------------------------------------------
//		MultiFileFindDone
// ---------------------------------------------------------------------------
//	Called from the multifilefind thread when it's done.

void
MFindThing::MultiFileFindDone(bool inFound)
{
	if (((fData.fBatch || ! inFound) && ! fData.fOnlyOne) &&
			(!fFindInNextFile || (fData.fWrap)))
		fCurrentMultiIndex = fStartMultiIndex = 0;
	fMultiFileThread = nil;
	fFindInNextFile = false;

	MFindWindow::GetFindWindow().MultiFileFindDone(inFound);

	if (inFound && fData.fBatch) {
		this->GetMessageWindow()->PostMessage(msgShowAndActivate);
		this->GetMessageWindow()->PostMessage(msgDoneWithMessageWindow);
	}
	if (! inFound)
		beep();
}

// ---------------------------------------------------------------------------
//		IncrementMultiIndex
// ---------------------------------------------------------------------------

void
MFindThing::IncrementMultiIndex()
{
	if (fData.fForward)
	{
		fCurrentMultiIndex++;
		if (fData.fWrap && fCurrentMultiIndex >= fAllList->CountItems())
			fCurrentMultiIndex = 0;
	}
	else
	{
		fCurrentMultiIndex--;
		if (fData.fWrap && fCurrentMultiIndex < 0 )
			fCurrentMultiIndex = fAllList->CountItems() - 1;
	}
}

// ---------------------------------------------------------------------------
//		ResetMultiFind
// ---------------------------------------------------------------------------

void
MFindThing::ResetMultiFind()
{
	MFindWindow::GetFindWindow().Lock();
	MFindWindow::GetFindWindow().ResetMultiFind();
	MFindWindow::GetFindWindow().Unlock();

	fCurrentMultiIndex = fStartMultiIndex = 0;
}

// ---------------------------------------------------------------------------
//		BlueRowChanged
// ---------------------------------------------------------------------------

void
MFindThing::BlueRowChanged()
{
	ASSERT(fListView);
	fCurrentMultiIndex = fStartMultiIndex = fListView->BlueRow();
}

// ---------------------------------------------------------------------------

const int32 kMaxFindStringInTitle = 32;

MMessageWindow*
MFindThing::GetMessageWindow()
{
	// cache the message window the first time we create it so that
	// the multiple file finds can all find the same one
	// The next time we come into the batch or multi-file find we
	// we create a new message window
	
	BString infoTitle = "Occurrences of \"";
	BString findString = fData.FindString;
	infoTitle += findString.RemoveSet("\t\n\r");
	infoTitle += "\"";
	
	if (fMessageWindow == nil) {
		BString windowTitle = "Find Results (";
		int32 origCount = findString.CountChars();
		findString.Truncate(kMaxFindStringInTitle);
		if (findString.CountChars() < origCount) {
			findString += B_UTF8_ELLIPSIS;
		}
		windowTitle += findString;
		windowTitle += ")";
		
		// it is ok if fMultiFileThread is nil (as it will be from DoBatchFind)
		BMessenger* threadHandler = fMultiFileThread ? new BMessenger(&MFindWindow::GetFindWindow()) : nil;
		fMessageWindow = new MLookupThreadInformationWindow(windowTitle.String(), 
															infoTitle.String(), 
															threadHandler);
	}
	
	return fMessageWindow;
}

