//
// TextListControl.h
//

#ifndef TEXT_LIST_CONTROL_H
#define TEXT_LIST_CONTROL_H

#include <Locker.h>
#include <TextControl.h>
#include <String.h>

class BChoiceList;
class BStringChoiceList;

namespace BPrivate {
	class TextListTextInput;
	class TextListWindow;
	class TextListView;
	class KeyedStringObjectList;
}

class BTextListControl : public BTextControl
{
public:
						BTextListControl(
							BRect frame,
							const char *name,
							const char *label,
							const char *initial_text,
							BMessage *message,
							BChoiceList *choiceList = NULL,
							bool ownChoiceList = true,
							uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE); 

						BTextListControl(BMessage *data);

	virtual				~BTextListControl();

	static BArchivable	*Instantiate(BMessage *data);
	virtual status_t	Archive(BMessage *data, bool deep = true) const;

	// SetChoiceList() deletes the old BChoiceList object,
	// take ownership of the new choice list if ownership flag is true,
	// and then invalidates the pop-up list.
	virtual void		SetChoiceList(BChoiceList *list, bool ownership = true);
	// ChoiceList returns a pointer to the current choice list
	BChoiceList*	ChoiceList() const;
	// ChoiceListChanged should be called whenever an item in the choice list changes
	// so that BTextListControl can perform the proper updating.  BStringChoiceList
	// calls this method on its owner for all Add/Remove operations
	virtual void		ChoiceListChanged();

	// Select changes the list selection to the specified index, and if the
	// changeTextSelection flag is true, changes the text in the TextView to
	// the value at index and selects all the text in the TextView.  If index
	// is negative, the current selection will be deselected.
	virtual void		SelectByIndex(int32 index, bool changeTextSelection = false);
	// Same as above function, but takes a key instead of an index
	virtual void		SelectByKey(int32 key, bool changeTextSelection = false);
	
	// Returns the key of the current selection, or a negative value
	int32				CurrentSelection() const;
	
	// Enables or disables auto-completion
	virtual void		SetAutoCompletion(bool enabled);
	bool				IsAutoCompletionEnabled() const;

	// Sets the number of choices to display at once in the popup list.  This effectively
	// sets the maximum vertical size of the popup list window.
	virtual void		SetMaxVisibleItems(int32 count);
	int32				MaxVisibleItems() const;
	
	// Specifies whether or not to display only choices that match in the popup list
	virtual void		SetAutoPrune(bool enabled);
	bool				IsAutoPruneEnabled() const;
	
	virtual void		SetEnabled(bool enabled);
	virtual	void		SetDivider(float divide);
	virtual	void		FrameMoved(BPoint new_position);
	virtual	void		FrameResized(float new_width, float new_height);
	virtual	void		WindowActivated(bool active);

	virtual void		Draw(BRect update);
	virtual void		MessageReceived(BMessage *msg);
	virtual void		KeyDown(const char *bytes, int32 numBytes);
	virtual void		MouseDown(BPoint);
	virtual void		MouseUp(BPoint);
	virtual void		MouseMoved(BPoint, uint32, const BMessage*);
	virtual void		AttachedToWindow();
	virtual void		DetachedFromWindow();
	virtual void		SetFlags(uint32 flags);
	virtual	void		MakeFocus(bool focusState = true);
	virtual void		SetFont(const BFont *font, uint32 properties = B_FONT_ALL);	
	virtual	void		GetPreferredSize(float *width, float *height);

	virtual BHandler	*ResolveSpecifier(BMessage *msg,
									int32 index,
									BMessage *specifier,
									int32 form,
									const char *property);
	virtual status_t	GetSupportedSuites(BMessage *data);

/*----- Private or reserved -----------------------------------------*/	

	virtual status_t	Perform(perform_code d, void *arg);
		
private:
	friend class BPrivate::TextListTextInput;
	friend class BPrivate::TextListWindow;
	friend class BPrivate::TextListView;

	virtual	void			_ReservedTextListControl1();
	virtual	void			_ReservedTextListControl2();
	virtual	void			_ReservedTextListControl3();
	virtual	void			_ReservedTextListControl4();
	virtual	void			_ReservedTextListControl5();
	virtual	void			_ReservedTextListControl6();
	virtual	void			_ReservedTextListControl7();
	virtual	void			_ReservedTextListControl8();
	virtual	void			_ReservedTextListControl9();
	virtual	void			_ReservedTextListControl10();
	virtual	void			_ReservedTextListControl11();
	virtual	void			_ReservedTextListControl12();

	void				InitData(BMessage *data);
	void				TryAutoComplete();
	void				BuildMatchList();
	int32				MatchListIndexForKey(int32 key);
	bool				ShowChoiceWindow();
	bool				HideChoiceWindow(bool invoked);
	BPrivate::TextListWindow*	CreateChoiceWindow();
	
	BString				fUserText;
	BRect				fButtonRect;
	BChoiceList*		fChoiceList;
	BPrivate::TextListWindow*	fChoiceWindow;		
	BPrivate::KeyedStringObjectList*	fMatchList;
	int32				fCompletionIndex;
	int32				fSelected;
	int32				fSelectedIndex;
	int32				fMaxVisible;
	int32				fTextEnd;
	bool				fTrackingButtonDown;
	bool				fAutoComplete;
	bool				fButtonDepressed;
	bool				fDepressedWhenClicked;
	bool				fAutoPrune;
	bool				fOwnsChoiceList;
	
	uint8				_reserved_8[2];
	uint32				_reserved_32[8];
};

// -------------------------------------------------------------------------------

// Abstract class provides an interface for BTextListControl to access possible choices.
// Choices are used for auto-completion and for showing the pop-up list
class BChoiceList : public BArchivable
{
public:
			BChoiceList();
			BChoiceList(BMessage *archive);

	virtual status_t Archive(BMessage *archive, bool deep = true) const;

	virtual ~BChoiceList();
	// Returns the text for the specified choice, and sets *key to the choice's key,
	// or does nothing and returns NULL if index is out of range.
	virtual const char*	GetChoiceAt(int32 index, int32 *key = NULL) const = 0;
	// Returns the text for the specified choice, and sets *index to the choice's index,
	// or does nothing and returns NULL if key is not found.
	virtual const char*	GetChoiceByKey(int32 key, int32 *index = NULL) const = 0;
	// Looks for a match at or after startIndex which contains a choice
	// that starts with prefix.  If a match is found, B_OK is returned,
	// matchIndex is set to the list index that should be selected, and completionText
	// is set to the text that should be appended to the text input.
	// If no match is found, a negative value is returned.
	virtual status_t	FindMatch(	const char *prefix,
									int32 startIndex,
									int32 *matchIndex,
									BString *completionText,
									int32 *matchKey) const = 0;
	// Returns the number of choices
	virtual int32		CountChoices() const = 0;

	void				SetOwner(BTextListControl *owner);
	BTextListControl*	Owner();

	// all access to BChoiceLists must occur when they are locked.
	bool				Lock();
	void				Unlock(); 

protected:
	BLocker				fLock;
	
private:
	virtual	void			_ReservedChoiceList1();
	virtual	void			_ReservedChoiceList2();
	virtual	void			_ReservedChoiceList3();
	virtual	void			_ReservedChoiceList4();
	virtual	void			_ReservedChoiceList5();
	virtual	void			_ReservedChoiceList6();
	virtual	void			_ReservedChoiceList7();
	virtual	void			_ReservedChoiceList8();

	BTextListControl*	fOwner;
	uint32 				_reserved[8];
};

// -------------------------------------------------------------------------------

// A BChoiceList that stores BStrings.  It keeps copies of each choice added,
// and frees the memory when the choices are removed.
class BStringChoiceList : public BChoiceList
{
public:
					BStringChoiceList();
					BStringChoiceList(BMessage *archive);
	virtual 		~BStringChoiceList();

	virtual status_t	Archive(BMessage *archive, bool deep = true) const;
	static BArchivable	*Instantiate(BMessage *data);
	
	virtual const char *GetChoiceAt(int32 index, int32 *key = NULL) const;
	virtual const char *GetChoiceByKey(int32 key, int32 *index = NULL) const;
	virtual status_t	FindMatch(	const char *prefix,
									int32 startIndex,
									int32 *matchIndex,
									BString *completionText,
									int32 *matchKey) const;
	virtual int32	CountChoices() const;

	// If you call AddChoice with a negative key value, a positive key value will
	// be generated and assigned.  You must have BStringChoiceList assign either
	// all values, or assign them all yourself.  Otherwise, you risk non-unique keys.
	status_t		AddChoice(const char *string, int32 key);
	status_t		AddChoice(const char *string, int32 key, int32 index);

	status_t		RemoveChoice(const char *string);
	status_t		RemoveChoiceAt(int32 index);
	status_t		RemoveChoiceByKey(int32 key);

	// Note: Sorting functionality should be added to the API.

private:
	virtual	void			_ReservedStringChoiceList1();
	virtual	void			_ReservedStringChoiceList2();
	virtual	void			_ReservedStringChoiceList3();
	virtual	void			_ReservedStringChoiceList4();
	virtual	void			_ReservedStringChoiceList5();
	virtual	void			_ReservedStringChoiceList6();

	BPrivate::KeyedStringObjectList*	fList;
	int32		fAutoKey;
	uint32		_reserved[4];
};

#endif // TEXT_LIST_CONTROL_H
