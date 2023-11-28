//========================================================================
//	MFindThing.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MFINDTHING_H
#define _MFINDTHING_H

class MFindThing;

#include "MFindData.h"
#include <SupportDefs.h>
#include <OS.h>

class MMultiFileListView;
class MTextWindow;
class MIDETextView;
class MSourceFileSet;
class MMessageWindow;
struct entry_ref;

const bool kDontWrap = false;
const bool kDoWrap = true;

class MFindThing
{
	friend class MMultiFindThread;

public:
								MFindThing();
		virtual					~MFindThing();

		bool					DoFindNext(
									MIDETextView& 	inTextView,
									bool			inBeepIfNotFound = true);
		bool					DoReplace(
									MIDETextView& 	inTextView);
		void					DoReplaceAndFind(
									MIDETextView& 	inTextView);
		void					DoReplaceAll(
									MIDETextView& 	inTextView,
									bool			inBeepIfNotFound = true);
		void					DoFindInNextFile();
		
		void					DoBatchFind(
									MTextWindow&		inWindow);
		void					DoMultiFileFind();
		void					CancelMultiFindThread();
		void					ResetMultiFind();
		void					BlueRowChanged();
		bool					CanFind();

		void					SetFindData(FindData& inData);
		void					SetFileList(MSourceFileSet*	inList);
		void					SetListView(MMultiFileListView*	inView);

private:

		MMultiFindThread*		fMultiFileThread;
		MMultiFileListView*		fListView;
		MSourceFileSet*			fAllList;

		int32					fCurrentMultiIndex;
		int32					fStartMultiIndex;
		bool					fFindInNextFile;
		FindData				fData;

		MMessageWindow*			fMessageWindow;
				
		static short			fRegExpError;
		
		bool					FindNext(
									const char* inText, 
									int32 		inTextLen, 
									int32 		inOffset, 
									int32& 		outOffset, 
									int32& 		outLength,
									bool		inWrap);

		bool					BatchFind(
									const char*		inText,
									int32			inLength,
									entry_ref&		inRef,
									const char *	inFileName,
									bool			inWrap);

		bool					ExecuteMultiFind(
									MMultiFindThread&	inFindThread);
		void					MultiFileFindDone(
									bool	inFound);
		bool					OpenAndReplaceAll(
									sem_id&				inSem,
									const entry_ref&	inRef);
		void					IncrementMultiIndex();

		bool					ConfirmSelectionValid(
									MIDETextView& 	inTextView);

		bool					ConfirmSelectionValid(
									const char*		inText,
									int32			inSelStart,
									int32			inSelEnd);
		bool					EqualText(
									const char* 	inString,
									const char* 	inText,
									bool 			inCaseInsensitive);

		bool					IsWhiteSpace(
									char 	inChar);
	
		int32					LineOf(
									const char* 	inText,
									int32			offset,
									int32			oldLineNumber,
									int32&			oldOffset);

		int32					LineEndOffset(
									const char*		inText,
									int32			inOffset,
									int32			inTextLen);


		MMessageWindow*			GetMessageWindow();

	static void 				HandleRegExpError(
									short inCode);
};

inline void 
MFindThing::SetFindData(
	FindData&	inData)
{
	fData = inData;
}

inline void 
MFindThing::SetFileList(MSourceFileSet*	inList)
{
	fAllList = inList;
}

inline void 
MFindThing::SetListView(
	MMultiFileListView*		inView)
{
	fListView = inView;
}

#endif
