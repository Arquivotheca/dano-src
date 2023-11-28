#ifndef _PATCHINFOWIND_H_
#define _PATCHINFOWIND_H_


// PatchInfoWind.h
#include "RList.h"
#include "SimpleListView.h"
#include <Rect.h>
#include <View.h>
#include <Window.h>
#include <Message.h>
#include <string.h>
#include <stdlib.h>



enum {
	M_DELETE_ORIGINALS = 'delO'
};



class ArchivePatchItem;

class PatchInfoWind : public BWindow
{
public:
	PatchInfoWind(RList<ArchivePatchItem *> *_patchList,
							  bool	*_continueInstall,
							  bool  *_deleteOriginals);

	virtual void	MessageReceived(BMessage *);
	virtual bool	QuitRequested();
			void	Go();
private:
	RList<ArchivePatchItem *> *patchList;
	bool			*continueInstall;
	bool			*deleteOriginals;
};


class PatchInfoView : public BView
{
public:
	PatchInfoView(BRect frame,
				RList<ArchivePatchItem *> *patchList);
	virtual void AllAttached();
};


////////////////////////////////////////////////////////////////

class StringItem : public ListItem
{
public:
	StringItem(const char *_string) :
		ListItem() {
			string = strdup(_string);
		}
	virtual ~StringItem() {
			free(string);
		}
	char *string;	
};

class PatchListView : public SimpleListView
{
public:
	PatchListView(BRect r,RList<StringItem *> *_itemList);
	~PatchListView();
	virtual void MouseDown(BPoint);
	virtual void DrawItem(BRect updateRect,
						long index,
						BRect *itemFrame = NULL);
	virtual void SelectionSet();
	virtual void Invoke(long index);
	
private:
	RList<StringItem *>		*patchList;
};

#endif
