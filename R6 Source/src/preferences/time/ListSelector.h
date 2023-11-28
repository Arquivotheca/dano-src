// **************************************************************************
//
//		This set of classes is not for public consumption
//		It has not been thoroughly tested and was designed
//			with a specific use in mind
//		User at your own risk
//
// **************************************************************************

#ifndef list_selector
#define list_selector

#include <Bitmap.h>
#include <List.h>
#include <Invoker.h>
#include <View.h>

// **************************************************************************

//	the selection list is a list of either
//	specificly named items
//	or a linear counter from floor to ceiling
//	
class TSelectionList : public BList {
public:
							TSelectionList(char* delimiter,
								BMessage* m=NULL);
							TSelectionList(int32 floor, int32 ceiling,
								char* delimiter=NULL, BMessage* m=NULL);
							~TSelectionList();

		//	blist methods
		//	haven't tested mixing of use of blist when list is counter
		void				AddItem(const char*);		
		const char*			ItemAt(int32 index) const;
		int32				CountItems() const;

		//	list item display
		char*				Delimiter() const;
		void				SetDelimiter(char*);
		
		void				SetAlignment(alignment);
		alignment			Alignment() const;
		
		//	max frame based on list contents
		BRect				Frame() const;
		void				SetFrame(BRect);

		//	current selection and text for selection
		const char*			SelectedText() const;				
		int32				Selection() const;
		void				SetSelection(int32);
		
		//	min and max for list
		//	set to constrain list
		void				SetFloor(int32);
		int32				Floor() const;
		void				SetCeiling(int32);
		int32				Ceiling() const;
		
		BMessage*			Message() const;
		void				SetMessage(BMessage*);
		
		//	toggle the 'counter' list vs item list
		//	definately haven't tested in a dynamic sense
		//	use on construction only
		void				SetCounter(bool);
		bool				IsCounter() const;
		
private:
		alignment			fAlignment;
		char*				fDelimiter;
		BRect				fFrame;
		int32				fSelection;
		BMessage*			fMessage;
		int32				fFloor;
		int32				fCeiling;
		bool				fIsCounter;
};

const short kMaxListCount = 4;
const short kLabelGap = 5;
const short kArrowWidth = 16;
const short kDelimiterPad = 6;

enum button {
	kNoButton=1,
	kUpButton,
	kDownButton
};

//	the list selector object has a TextControl like
//	border and display.
//	on its right edge are up and down arrows
//	its contents are based on TSelectorLists
//	increment and decrement and display are tied directly to
//		the list currently selected
class TListSelector : public BView, public BInvoker {
public:
							TListSelector(BRect, const char*, BMessage*);
virtual 					~TListSelector();

		//	view
		void				AttachedToWindow();
		void 				WindowActivated(bool state);
		void				MakeFocus(bool focusState = true);
		
		void				GetPreferredSize(float*, float*);
		void				ResizeToPreferred();

		void				Draw(BRect);
		void				DrawLabel();
		void				DrawBevel();
		BRect				BevelFrame() const;
		void				DrawFocusMark();
		void				DrawSelection();
		void				DrawButtons();
		
		void				MessageReceived(BMessage*);
		//		draws the appropriate button in the correct state
		void				PressButton(button);
virtual bool				HandleKeyDown();
virtual void				KeyDown(const char *key, int32 numBytes);
virtual void				MouseDown(BPoint);

		// control
		void				SetEnabled(bool);
		bool				IsEnabled() const;
		void				SetLabel(const char*);
		const char*			Label() const;
		status_t			Invoke(BMessage *msg = NULL);
		
		
		// buttons specific
		//		set the activity state of the button being pressed
		void				SetActive(bool);
		bool				Active() const;
		
		//		set which button is being pressed (true-up, false-down)
		void				SetButton(button);
		button				Button() const;

		void				SetSleepTime(int32);

		//	selection list
		//	returns/sets the index
		
		//		toggle hiliting of selected list 
		void				MakeSelected(bool);
		bool				Selected() const;
		
		//		set the selection for the target list 
		void				SetSelection(int32);
		//		get the index or text of the selection of the target list
		int32				Selection() const;
		const char*			SelectedText() const;
		//		same, for specific list
		void				SetSelection(int32 index, short which);
		int32				Selection(short which) const;
		const char*			SelectedText(short which) const;				
		
		//		constrain or expand the lists
		void				SetFloor(int32, short);
		int32				Floor(short) const;
		void				SetCeiling(int32, short);
		int32				Ceiling(short) const;
		
		//		sets the alignment of a list
		void				SetAlignment(alignment, short);
		alignment			Alignment(short) const;
		
		//		inc/dec selection based  on target list
		//			auto wrapping is handled only for these two
		void				Increment();
		void				Decrement();

		//		add a list to the end of the list of lists
		//		can't remove for now, so there
		void				AddList(TSelectionList*);
		void				AddList(int32 floor, int32 ceiling,
								char* delimiter=NULL, BMessage* m=NULL);
								
		//		similar blist methods
		int32				ItemCount() const;
		int32				ItemCount(short whichList) const;		
		const char*			ItemAt(int32 index) const;
								// note - the above actually sometimes allocates
								// and thus doesn't always return const char *
								// should clean this up and use GetStringAt(BString &result, int32 index) const;
								// or something like that
								// or at least just use char* ItemAt(int32 index)
		const char*			ItemAt(int32 index, short whichList) const;
		
		TSelectionList*		ListAt(short index) const;
		
		//		the target list, for manipulation
virtual	void				SetTargetList(short);
		short				TargetList() const;
		short				TargetListCount() const;
virtual void				NextTargetList();
virtual void				PreviousTargetList();

		BRect				UpArrowFrame() const;
		BRect				DownArrowFrame() const;
		
private:
		bool				fSelected;
		bool				fEnabled;
		bool				fActive;
		char*				fLabel;
		int32				fSleepTime;

		//	buttons
		bool				fNeedToInvert;
		button				fButton;
		
		BBitmap*			fUpArrow;
		BBitmap*			fHiliteUpArrow;
		BBitmap*			fDisabledUpArrow;

		BBitmap*			fDownArrow;
		BBitmap*			fHiliteDownArrow;
		BBitmap*			fDisabledDownArrow;
		
		short				fTargetList;
		short				fListCount;
		TSelectionList*		fList[kMaxListCount];
		
		BView*				fOffscreenView;
		BBitmap*			fOffscreenBits;
};

#endif
