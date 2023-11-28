#ifndef _HISTORY_H
#define _HISTORY_H

#include "URL.h"
#include "Resource.h"

const int32 kHistorySize = 56;  // Class size should be this

namespace Wagner {

class FrameSet;

class History {
public:
	struct FramePath {
		FramePath(int32 max = 0);
		~FramePath(void);
		
		FramePath& operator=(const FramePath& src);

		int32* indices;
		int32 depth;
		int32 maxDepth;
	};

	History(void);				// This version is deprecated
	History(const char* name);	// Opens a named history instance
	virtual ~History(void);

	void SetFrameData(const FramePath& framePath, const BMessage& frameData);
	bool FrameExists(const FramePath& framePath) const;

	void RemoveCurrentEntry(void);
	void Checkpoint(void);
	void Clear(void);                // Clears this instance
	void Goto(const FramePath& framePath, const URL& url, GroupID zone);

	void GoBack(BMessage* msg);
	void GoForward(BMessage* msg);
	void GoOffset(BMessage* msg, int32 offset, bool retrieve_info_only = false);
	bool CanGoBack(void) const;
	bool CanGoForward(void) const;
	bool CanGoOffset(int32 offset) const;
	void BackFrameDelta(FramePath* framePath) const;
	void ForwardFrameDelta(FramePath* framePath) const;
	void OffsetFrameDelta(FramePath* framePath, int32 offset) const;
	
	int32 CountItems(void) const;
	
	void Dump(void);
	bool WasRestored(void) const;
	
private:
	struct HistoryEntry {
		FramePath framePath;
		FrameSet* frameSet;
		HistoryEntry* next;
		HistoryEntry* prev;
	};

	void InitData(const char* name);  // Does constructor work
	static void DeleteSet(FrameSet* set);
	static void CopySet(FrameSet* dest, FrameSet* src);
	FrameSet* WalkPath(const FramePath& framePath, bool extend);
	void DoCheckpoint(bool full);
	
	bool			fCheckpoint;
	HistoryEntry	fDummyHead;
	HistoryEntry*	fCurrentEntry;
	int32			fNumEntries;
	BString			name;
	
	uint8           pad[4];      // Force class to be kHistorySize bytes

	// Interface for storing history in the binder
	
	void BinderBackward(void);
	void BinderClose(void);
	void BinderForward(void);
	void BinderOffset(int32 offset);
	void BinderOpen(void);
	void BinderPush(const BMessage* msg);
	void BinderWrite(const BMessage* msg);
	
	int fDriverFD;
	bool fRestored;
};

}

#endif
