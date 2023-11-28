// InstallListView.h
#ifndef _INSTALLLISTVIEW_H_
#define _INSTALLLISTVIEW_H_

#include "SimpleListView.h"
struct node_ref;
class BQuery;

class InstallListView : public SimpleListView
{
public:
	InstallListView(	BRect r,
						BHandler *_fTarget );

virtual				~InstallListView();
virtual void		AttachedToWindow();
virtual void		DrawItem(BRect updt,
							long item,
							BRect *itemFrame = NULL);

virtual void		HighlightItem(bool on, long index, BRect *iFrame);

virtual void		SelectionSet();
virtual void		Invoke(long index);
virtual void		MessageReceived(BMessage *msg);
		void		RemoveNode(node_ref *r);
private:
	BHandler		*fTarget;
	const	long	IHeight;
	ino_t			directoryID;
	BQuery			*query;
	RList<entry_ref *>	fWatchList;
	
	status_t 		WatchEntry(entry_ref *ref, bool start);
	bool			StillDownloading(entry_ref *ref);
	void			InsertEntry(entry_ref &ref, ino_t node);
	status_t		GetMsgEntry(BMessage *msg, entry_ref &ref, const char *opcode = "directory");
};

enum {
	M_ITEMS_SELECTED	= 'ISel'
};


#endif

