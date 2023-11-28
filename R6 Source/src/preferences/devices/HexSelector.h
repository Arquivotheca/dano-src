#ifndef hex_selector
#define hex_selector

#include "ListSelector.h"

char* PadAlign(int32 i, int32 align, bool prefix);

// **************************************************************************

enum list_type {
	kIntegerList=0,
	kHexList
};

class THexList : public TSelectionList {
public:
							THexList(char* delimiter,
								BMessage* m=NULL,
								list_type type=kIntegerList);
							THexList(int32 floor, int32 ceiling,
								char* delimiter=NULL, BMessage* m=NULL,
								list_type type=kIntegerList);
							~THexList();

		const char*			ItemAt(int32 index) const;
		
		list_type			ListType() const { return fListType; }
private:
		list_type			fListType;
};

class THexSelector : public TListSelector {
public:
							THexSelector(BRect, const char*, BMessage*,
								bool fixed=false);
virtual 					~THexSelector();

		//	view
		void				AttachedToWindow();
		void				ResizeToPreferred();
		
		void				DrawSelection();

		bool				DoTypeAhead(const char *bytes, int32 numBytes);
		void				DoTypeAheadSelection();
		
		void				KeyDown(const char *key, int32 numBytes);
		void				MouseDown(BPoint);

		void				AddList(THexList*);
		void				AddList(int32 floor, int32 ceiling,
								char* delimiter=NULL, BMessage* m=NULL);

		void				SetTargetList(short);
		void				NextTargetList();
		void				PreviousTargetList();
		
		void				SetShowSelection(bool state) { fShowSelection = state; }
		bool				ShowSelection() { return fShowSelection; }
private:
		bool				fFixed;
		bool				fShowSelection;
		
		char				fKeyBuffer[3];
		bigtime_t			fLastTime;
};

#endif
