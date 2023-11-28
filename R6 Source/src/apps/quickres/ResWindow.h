#ifndef QUICKRES_RES_WINDOW_H
#define QUICKRES_RES_WINDOW_H

#include "EditWindow.h"

#include <View.h>

#include <ResourceEditor.h>
#include <ResourceRoster.h>
#include <ToolTipHook.h>
#include <UndoContext.h>

#include <experimental/DocWindow.h>
#include <experimental/WindowRoster.h>
#include <experimental/MultiLocker.h>

#include <vector>

class BTextControl;
class BMenuField;
class BButton;
class BView;
class BMessageRunner;
class ResRow;
class ResUndoEntry;
class CollectionUndoEntry;
class ResEntry;
class ColumnContainer;
struct entry_ref;
class BClipboard;

namespace BPrivate {
class ResourceListView;
}
using namespace BPrivate;

namespace BExperimental {
	class DividerControl;
}
using namespace BExperimental;

enum {
	kMergeRequest = 'mgrq',
	kMergeResources = 'mrge'
};

class ResWindow : public DocWindow, public BResourceCollection,
				  public BToolTipFilter
{
public:
	ResWindow(WindowRoster *wr, entry_ref *ref, const char *title,
		window_look look, window_feel feel, uint32 flags, uint32 workspace);

	/*
	** WINDOW METHODS
	**
	** These methods do not touch the resource data, so there is
	** no need to worry about locking it.
	*/
	
	BFilePanel*	CreateSavePanel() const;
	thread_id	Run();
	void		Quit();
	bool		QuitRequested();
	void		MenusBeginning();
	void		MessageReceived(BMessage *msg);
	
	void		WindowFrame(BRect *proposed);
	void		LoadSettings();
	void		SaveSettings() const;
	
	/*
	** DATA MANIPULATION METHODS
	**
	** Using these methods must follow proper read/write locking
	** of the resource data, as described below.
	*/
	
	// Methods that automatically acquire read access.
	status_t		Save(BEntry* e, const BMessage* args = 0);
	
	// Methods that automatically acquire write access.
	status_t		Load(BEntry* e)					{ return Load(e, false); }
	status_t		Load(BEntry* e, bool merge);
	status_t		Merge(BEntry* e);
	virtual void	Cut(BClipboard* clipboard);
	virtual void	Paste(BClipboard* clipboard);
	void			Clear();

	// Methods that automatically acquire read access.
	virtual void	Copy(BClipboard* clipboard);
	
	// Methods that must be called in a read locked context.
	status_t		SaveAttributes(BEntry* e);
	status_t		SaveBinary(BEntry* e);
	status_t		SaveSource(BEntry* e);
	
	// BResourceCollection interface
	virtual status_t	GetNextItem(size_t *inout_cookie, BResourceHandle* out_item) const;
	virtual status_t 	AddItem(BResourceHandle* out_item,
								type_code type, int32 id, const char* name = 0,
								const void *data = 0, size_t len = 0,
								bool make_selected = true,
								conflict_resolution resolution = B_ASK_USER);
	virtual status_t	FindItem(BResourceHandle* out_item,
								 type_code type, int32 id, const char* name = 0) const;
	virtual status_t	RemoveItem(BResourceHandle& item);
	virtual status_t	SetCurrentItem(const BResourceHandle& item);
	virtual BResourceHandle CurrentItem() const;
	virtual int32		UniqueID(int32 smallest = 1) const;
	virtual int32		UniqueIDForType(type_code type, int32 smallest = 1) const ;
	virtual int32		NextID(bool fromCurrent=true) const;
	virtual int32		NextIDForType(type_code type, bool fromCurrent = true) const;
	virtual BUndoContext* UndoContext();
	virtual const BUndoContext* UndoContext() const;
	
	virtual status_t	AddEntry(ResEntry* e, bool make_selected=true,
								 conflict_resolution resolution = B_ASK_USER);
	
private:
	typedef BWindow inherited;
	
	friend class ResRow;
	friend class ResUndoEntry;
	friend class CollectionUndoEntry;
	
	/*
	** DATA MANIPULATION METHODS
	**
	** Using these methods must follow proper read/write locking
	** of the resource data, as described below.
	*/
	
	// Methods that automatically acquire read access.
	void		UpdateControlState();
	
	// Methods that must be called with the window locked.
	void		SyncResList();
	void		SortItems();
	void		UpdateRow(const BResourceAddonBase& acc,
						  ResRow* row, uint32 changes, bool force=false);
	
	// SPECIAL: This method automatically acquires read or write access,
	// as needed.
	void		UpdateEditorVisibility();
	
	// BResourceCollection interface
	virtual status_t	ReadLockCollection() const;
	virtual status_t	ReadUnlockCollection() const;
	virtual status_t	WriteLockCollection(const char* name = 0);
	virtual status_t	WriteUnlockCollection();
	virtual void		EntryChanged(BPrivate::ResourceEntry* e);
	
	// Don't hide this DocWindow method.
	virtual void		EntryChanged(BMessage *msg)	{ DocWindow::EntryChanged(msg); }
	
	// Methods for undo state -- must be called in a write locked
	// context.
	bool		PerformAdd(BResourceHandle item);
	bool		PerformRemove(BResourceHandle item);
	
	// Assertions for holding read and write locks.
	bool AssertReadLock(const char* context) const;
	bool AssertWriteLock(const char* context) const;
	
	/*
	** WINDOW METHODS
	**
	** These methods do not touch the resource data, so there is
	** no need to worry about locking it.
	*/
	
	// Bring up merge resources file requester.
	void		RequestMerge();
	
	// Data being edited, protected by the fDataLock read/write lock.
	
	BMultiLocker			fDataLock;
	BResourceRoster			fAddons;
	BUndoContext			fUndo;
	int32					fReadLocks;
	vector<BResourceHandle>	fItems;
	vector<BResourceHandle> fChangedItems;
	vector<BResourceHandle>	fDeletedAttrs;
	BResourceHandle			fCurItem;
	bool					fWriteEnding;
	bool					fChangedSelection;
	
	EditWindowController	fController;
	
	// Communication back to window about changes in collection.
	
	vector<BResourceHandle> fUpdatedItems;
	vector<uint32>			fUpdatedChanges;
	vector<BResourceHandle> fRemovedItems;
	
	// Display data, protected by the window lock.  Note that you must
	// ALWAYS acquire locks in the order of window -> data.
	
	ResourceListView		*fResList;
	
	ColumnContainer			*fColumns;
	DividerControl			*fDivider;
	
	BMessenger				fAddWindow;
	
	BView					*fRoot;
	BView					*fContainer;
	
	BMenuField				*fAddField;
	BMenuField				*fShowField;
	BButton					*fRemoveButton;
	BMenuItem				*fUndoItem;
	BMenuItem				*fRedoItem;
	BMenuItem				*fCutItem;
	BMenuItem				*fCopyItem;
	BMenuItem				*fPasteItem;
	BMenuItem				*fClearItem;
	BMenuItem				*fAddItem;
	BMenuItem				*fDupItem;
	BMenuItem				*fEditInplaceItem;
	BMenuItem				*fEditWindowItem;
	int32					number;

	BMessage				fWindowConfiguration;
	int32					fSaveFormat;
	bool					fAttributeSettings;
	bool					fBodyChanged;
	
	enum resolution_reply {
		kNotAsked,
		kAskEvery,
		kMoveAll,
		kDeleteAll
	};
	
	resolution_reply		fConflictResolution;
	BFilePanel*				fMergePanel;
};

#endif
