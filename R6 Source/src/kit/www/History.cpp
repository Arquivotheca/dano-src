#include <Debug.h>
#include <string.h>
#include <Message.h>
#include "History.h"
#include "history.h"
#include "parameters.h"
#include "StringBuffer.h"

// #define DEBUG_OUTPUT    // un-comment for printf() output

using namespace Wagner;

namespace Wagner {
struct FrameSet {
	FrameSet(void);
	
	void		AddToMessage(BMessage* message);
	void 		Dump(void);
	static void	DumpSet(FrameSet* set, int depth);
	void		RestoreFromMessage(BMessage* message);
	
	URL			url;
	GroupID		groupID;
	int32 		numSubFrames;
	FrameSet**	subFrames;
	BMessage*	frameData;
}; 
}

FrameSet::FrameSet(void)
	:	numSubFrames(0),
		subFrames(0),
		frameData(NULL)
{
}

void FrameSet::AddToMessage(BMessage* message)
{
	// Add my URL
	url.AddToMessage("url", message);
	message->AddInt32("group_id", (int32) groupID);
	if(frameData) message->AddMessage("frame_data", frameData);

	// Add my subframes
	for (int i = 0; i < numSubFrames; i++) {
		BMessage msg;
		if (subFrames[i])
			subFrames[i]->AddToMessage(&msg);
			
		message->AddMessage("frame_set", &msg);
	}
}

void FrameSet::Dump(void)
{
	DumpSet(this, 0);
}

void FrameSet::DumpSet(FrameSet* set, int depth)
{
	if (set == 0)
		return;
		
	for (int i = 0; i < depth; i++)
		printf("   ");
		
	StringBuffer tmp;
	tmp << set->url;
	printf("%s\n", tmp.String());
	for (int i = 0; i < set->numSubFrames; i++)
		DumpSet(set->subFrames[i], depth + 1);
}

void FrameSet::RestoreFromMessage(BMessage* message)
{
	url.ExtractFromMessage("url", message);
	groupID = (GroupID)message->FindInt32("group_id");

	if (message->HasMessage("frame_data")) {
		delete frameData;
		frameData = new BMessage();
		message->FindMessage("frame_data", frameData);
	}

	type_code type;
	message->GetInfo("frame_set", &type, &numSubFrames);
	subFrames = new FrameSet*[numSubFrames];
	for (int32 i = 0; i < numSubFrames; i++) {
		BMessage subMsg;
		message->FindMessage("frame_set", i, &subMsg);
		subFrames[i] = new FrameSet;
		subFrames[i]->RestoreFromMessage(&subMsg);
	}
}

History::FramePath::FramePath(int32 max) :
	indices(NULL),
	depth(0),
	maxDepth(max)
{
	if(maxDepth > 0) {
		indices = new int32[maxDepth];
	}
}

History::FramePath::~FramePath()
{
	delete [] indices;
}

History::FramePath &History::FramePath::operator=(const FramePath &src)
{
	if (src.depth > 0) {
		if (maxDepth < src.maxDepth) {
			delete [] indices;
			indices = new int32[src.maxDepth];
			maxDepth = src.maxDepth;
		}
		memcpy(indices, src.indices, src.depth*sizeof(int32));
	}
	depth = src.depth;
	return *this;
}


History::History(void)
{
	#ifdef DEBUG_OUTPUT
	printf("History constructor\n");
	#endif
	
	InitData(NULL);
}

History::History(const char* name)
{
	#ifdef DEBUG_OUTPUT
	printf("History constructor\n");
	#endif
	
	InitData(name);
}

History::~History(void)
{
	#ifdef DEBUG_OUTPUT
	printf("History destructor\n");
	#endif
	
	while (fDummyHead.next != &fDummyHead) {
		HistoryEntry *entry = fDummyHead.next;
		entry->next->prev = entry->prev;
		entry->prev->next = entry->next;
		DeleteSet(entry->frameSet);
		delete entry;
	}
	
	BinderClose();
}

void History::BackFrameDelta(FramePath* framePath) const
{
	*framePath = fCurrentEntry->framePath;
}

void History::BinderBackward(void)
{
	history_back(fDriverFD);
}

void History::BinderClose(void)
{

}

void History::BinderForward(void)
{
	history_forward(fDriverFD);
}

void History::BinderOffset(int32 offset)
{
	if(offset > 0) {
		for(int32 c=0;c<offset;c++)
			history_forward(fDriverFD);
	} else if(offset < 0) {	
		for(int32 c=0;c<-offset;c++)
			history_back(fDriverFD);
	}
}

void History::BinderOpen(void)
{
	fDriverFD = open("/dev/misc/history_cache", O_RDONLY);

	// Restore history	
	int32 historyCount = 0;
	BMessage msg;
	while (history_back(fDriverFD) >= 0)
		historyCount++;

	if (historyCount > 0)
		fRestored = true;

	while (historyCount-- > 0) {
		ssize_t size = (ssize_t) history_peek(fDriverFD);
		char *buf = (char*)malloc(size);
		if (history_read(fDriverFD, buf, size) < 0) {
			PRINT(("error restoring from history driver"));
			break;
		}

		BMessage msg;
		msg.Unflatten(buf);
		free(buf);

		FrameSet *fs = new FrameSet;
		fs->RestoreFromMessage(&msg);

		fCurrentEntry = new HistoryEntry;
		fCurrentEntry->next = &fDummyHead;
		fCurrentEntry->prev = fDummyHead.prev;
		fCurrentEntry->next->prev = fCurrentEntry;
		fCurrentEntry->prev->next = fCurrentEntry;
		fCurrentEntry->frameSet = fs;
		fNumEntries++;
		
		history_forward(fDriverFD);
	}	
}

void History::BinderPush(const BMessage* msg)
{
	ssize_t flSize = msg->FlattenedSize();
	char *buf = (char*)malloc(flSize);
	msg->Flatten(buf, flSize);
	history_push(fDriverFD, buf, flSize);
	free(buf);
}

void History::BinderWrite(const BMessage* msg)
{
	history_back(fDriverFD);
	BinderPush(msg);
}

bool History::CanGoBack(void) const
{
	return fCurrentEntry && fCurrentEntry->prev != &fDummyHead;
}

bool History::CanGoForward(void) const
{
	return fCurrentEntry && fCurrentEntry->next != &fDummyHead;
}

bool History::CanGoOffset(int32 offset) const
{
	if (offset > 0) {
		HistoryEntry *currentEntry = fCurrentEntry;
		for(int32 c=0;c<offset;c++) {
			if(!currentEntry || (currentEntry->next == &fDummyHead))
				return false;
			else
				currentEntry = currentEntry->next;
		} 		
		return true;
	} else if (offset < 0) {
		HistoryEntry *currentEntry = fCurrentEntry;
		for(int32 c=0;c<(-offset);c++) {
			if(!currentEntry || (currentEntry->prev == &fDummyHead))
				return false;
			else
				currentEntry = currentEntry->prev;
		}
		return true;
	}
	else {
		if(fCurrentEntry)
			return true;
		else
			return false;
	}
}

void History::Checkpoint(void)
{
	// Creates a new history entry
	
	if (fCurrentEntry && fCurrentEntry != &fDummyHead) {
		if (fCurrentEntry->frameSet) {
			BMessage msg;
			fCurrentEntry->frameSet->AddToMessage(&msg);
			BinderPush(&msg);
		}
	}

	if (fCurrentEntry && fCurrentEntry != fDummyHead.prev) {
		// We've gone back, then opened a page.  Delete forward pages.
		fCurrentEntry = fCurrentEntry->next;
		while (fCurrentEntry != &fDummyHead) {
			HistoryEntry *entry = fCurrentEntry;
			fCurrentEntry = fCurrentEntry->next;
			
			entry->next->prev = entry->prev;
			entry->prev->next = entry->next;
			DeleteSet(entry->frameSet);
			delete entry;
			fNumEntries--;
		}
	}
	
	// Add this to the end of the list
	if (fNumEntries++ > kMaxHistoryEntries) {
		// Remove an entry from the beginning of the list if there are too many
		HistoryEntry *del = fDummyHead.next;
		del->next->prev = del->prev;
		del->prev->next = del->next;
		DeleteSet(del->frameSet);
		delete del;
		fNumEntries--;
	}
	

	fCurrentEntry = new HistoryEntry;
	fCurrentEntry->next = &fDummyHead;
	fCurrentEntry->prev = fDummyHead.prev;
	fCurrentEntry->next->prev = fCurrentEntry;
	fCurrentEntry->prev->next = fCurrentEntry;
	fCurrentEntry->frameSet = new FrameSet;	// This will get copied/created in Goto
	fCheckpoint = true;
}

void History::Clear(void)
{
	while (fDummyHead.next != &fDummyHead) {
		HistoryEntry *entry = fDummyHead.next;
		entry->next->prev = entry->prev;
		entry->prev->next = entry->next;
		DeleteSet(entry->frameSet);
		delete entry;
	}

	history_clear(fDriverFD);
  
	fCheckpoint = false,
	fCurrentEntry = 0;
	fNumEntries = 0;
	fRestored = false;
		
	fDummyHead.next = &fDummyHead;
	fDummyHead.prev = &fDummyHead;
	fDummyHead.frameSet = NULL;
	
	Checkpoint();
}

void History::CopySet(FrameSet* dest, FrameSet* src)
{
	dest->url = src->url;
	dest->groupID = src->groupID;
	dest->numSubFrames = src->numSubFrames;
	delete[] dest->subFrames;
	delete dest->frameData;
	
	if (dest->numSubFrames == 0)
		dest->subFrames = 0;
	else {
		dest->subFrames = new FrameSet*[src->numSubFrames];
		for (int i = 0; i < src->numSubFrames; i++) {
			if (src->subFrames[i]) {
				dest->subFrames[i] = new FrameSet;
				CopySet(dest->subFrames[i], src->subFrames[i]);
			} else
				dest->subFrames[i] = 0;
		}
	}
}

int32 History::CountItems(void) const
{
	int32 c=0;
	if(fCurrentEntry) {
		c++;
		HistoryEntry *currentEntry = fCurrentEntry;
		for(;currentEntry->prev && currentEntry->prev!=&fDummyHead ;currentEntry=currentEntry->prev) {}
		for(;currentEntry->next && currentEntry->next!=&fDummyHead ;currentEntry=currentEntry->next,c++) { }
	}
	return c;
}

void History::DeleteSet(FrameSet* set)
{
	if (set == 0)
		return;

	for (int i = 0; i < set->numSubFrames; i++)
		DeleteSet(set->subFrames[i]);

	delete set->frameData;
	delete [] set->subFrames;
	delete set;
}

void History::DoCheckpoint(bool full)
{
	// This is the first rebranch call the last checkpoint.  Since we're guaranteed
	// parent->child ordering, we know now whether this is a top level load or
	// subframe load.  Allocate a new entry.
	if (!(fCurrentEntry->frameSet)) // May have already been allocated in Checkpoint()
		fCurrentEntry->frameSet = new FrameSet; // Allocate a new frame set.

	if (!(full || fCurrentEntry->prev == &fDummyHead)) {
		// Copy the frame set from the last page because this is a subframe
		// load
		CopySet(fCurrentEntry->frameSet, fCurrentEntry->prev->frameSet);
	}
	
	fCheckpoint = false;
}

void History::Dump(void)
{
	printf("history\n");
	for (HistoryEntry *entry = fDummyHead.next; entry != &fDummyHead; entry = entry->next) {
		printf("--");
		if (entry->frameSet)
			entry->frameSet->Dump();
		else
			printf("NULL\n");
	}
}

void History::ForwardFrameDelta(FramePath* framePath) const
{
	*framePath = fCurrentEntry->next->framePath;
}

bool History::FrameExists(const FramePath& framePath) const
{
	return const_cast<History*>(this)->WalkPath(framePath, false) != NULL;
}

void History::GoBack(BMessage* msg)
{
#if DEBUG
	if (!CanGoBack())
		debugger("can't go backward");
#endif
	
	BinderBackward();
	fCurrentEntry = fCurrentEntry->prev;
	fCurrentEntry->frameSet->AddToMessage(msg);
}

void History::GoForward(BMessage* msg)
{
#if DEBUG
	if (!CanGoForward())
		debugger("can't go forward");;
#endif
	
	BinderForward();
	fCurrentEntry = fCurrentEntry->next;
	fCurrentEntry->frameSet->AddToMessage(msg);
}

void History::GoOffset(BMessage* msg, int32 offset, bool retrieve_info_only)
{
#if DEBUG
	if (!CanGoOffset(offset))
		debugger("can't go offset");;
#endif
	
	if(!retrieve_info_only) {
		BinderOffset(offset);
	}

	HistoryEntry *currentEntry = fCurrentEntry;
	if(offset > 0) {
		for(int32 c=0;c<offset;c++)
			currentEntry = currentEntry->next;
	} else {
		for(int32 c=0;c<(-offset);c++)
			currentEntry = currentEntry->prev;
	}

	currentEntry->frameSet->AddToMessage(msg);	

	if(!retrieve_info_only)
		fCurrentEntry = currentEntry;
}

void History::Goto(const FramePath& framePath, const URL& url, GroupID groupID)
{
	// Set the url for a frame entry
	
	if (fCheckpoint)
		DoCheckpoint(framePath.depth == 0);

	FrameSet *set = WalkPath(framePath, true);
	if (!set)
		return; 	// Apparently I've been disconnected from my parent

	set->url = url;
	set->groupID = groupID;
	if (set->subFrames != 0) {
		// sub frames are already allocated for this set.  Delete them.
		for (int i = 0; i < set->numSubFrames; i++)
			DeleteSet(set->subFrames[i]);

		delete [] set->subFrames;
		set->subFrames = 0;
		set->numSubFrames = 0;
	}

	// Update history driver
	if (fCurrentEntry && fCurrentEntry != &fDummyHead && fCurrentEntry->frameSet) {
		BMessage msg;
		fCurrentEntry->frameSet->AddToMessage(&msg);
		BinderWrite(&msg);
	}
}

void History::InitData(const char* name)
{
	if (!name || !name[0]) name = "default";
	this->name.SetTo(name);
	
	fCheckpoint = false,
	fCurrentEntry = 0;
	fNumEntries = 0;
	fRestored = false;
		
	fDummyHead.next = &fDummyHead;
	fDummyHead.prev = &fDummyHead;
	fDummyHead.frameSet = NULL;
	
	int32  size = sizeof (History);
	
	if (size != kHistorySize)
	  printf("Warning, History size is %ld, should be %ld\n",
	   size, kHistorySize);
	
	BinderOpen();
	Checkpoint();
}

void History::OffsetFrameDelta(FramePath* framePath, int32 offset) const
{
	if(fCurrentEntry)
		*framePath = fCurrentEntry->framePath;
	if(offset>0) {
		HistoryEntry *currentEntry = fCurrentEntry;
		for(int32 c=0;c<offset;c++) {
			if(!currentEntry || (currentEntry->next == &fDummyHead))
				return;
			else
				currentEntry = currentEntry->next;
		} 		
		*framePath = currentEntry->framePath;
	} else {
		HistoryEntry *currentEntry = fCurrentEntry;
		for(int32 c=0;c<((-offset)-1);c++) {
			if(!currentEntry || (currentEntry->prev == &fDummyHead))
				return;
			else
				currentEntry = currentEntry->prev;
		} 		
		*framePath = currentEntry->framePath;
	}
}

void History::RemoveCurrentEntry(void)
{
	if (fCurrentEntry && fCurrentEntry != &fDummyHead) {
		HistoryEntry *rip = fCurrentEntry;
		fCurrentEntry = fCurrentEntry->prev;

		DeleteSet(rip->frameSet);
		rip->next->prev = rip->prev;
		rip->prev->next = rip->next;
		delete rip;
		fNumEntries--;
	}
}

void History::SetFrameData(const FramePath &framePath, const BMessage &frameData)
{
	FrameSet *set = WalkPath(framePath, false);
	if (set == NULL) {
		PRINT(("WARNING!  WalkPath() returned NULL!\n"));
		return;
	};

	delete set->frameData;
	set->frameData = new BMessage(frameData);

	BMessage msg;
	fCurrentEntry->frameSet->AddToMessage(&msg);
	BinderWrite(&msg);
}

FrameSet* History::WalkPath(const FramePath &framePath, bool extend)
{
	if (fCurrentEntry == &fDummyHead) return NULL;

	FrameSet **set = &fCurrentEntry->frameSet;
	if (framePath.depth > 0) {
		// Walk the current frame structure.
		for (int i = 0; i < framePath.depth; i++) {
			if (framePath.indices[i] >= (*set)->numSubFrames) {
				if (extend) {
					int j = 0;
					int32 newNumSubFrames = framePath.indices[i]+1;
					FrameSet ** newSubFrames = new FrameSet*[newNumSubFrames];
					for (;j < (*set)->numSubFrames; j++)	newSubFrames[j] = (*set)->subFrames[j];
					for (;j < newNumSubFrames;   j++)		newSubFrames[j] = NULL;
					delete [] (*set)->subFrames;
					(*set)->subFrames = newSubFrames;
					(*set)->numSubFrames = newNumSubFrames;
				}
				else {
					return NULL;
				}
			};

			set = &(*set)->subFrames[framePath.indices[i]];		
		}

		if (*set == NULL && extend) *set = new FrameSet;
	}

	return *set;
}

bool History::WasRestored(void) const
{
	return fRestored;
}

