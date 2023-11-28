#ifndef _CTREEITEM_H_
#define _CTREEITEM_H_

#include "TreeView.h"

// checkmark tree item
class CTreeItem : public TreeItem
{
public:
	CTreeItem(const char *name, BBitmap *icon);
	
	virtual ~CTreeItem();
	
	virtual void	DrawLabel( TreeView* owner);
	virtual void	DrawIcon( TreeView *owner);
	
	virtual void	SetIcon(BBitmap *);
	
	virtual float	Height();
			void	CollapseTo(long level, long maxcount);
	
	// draw and animate triangle tabs
	virtual void	DrawPlusBox(TreeView *owner);
	virtual void	DrawMinusBox(TreeView *owner);
	virtual void	AnimateBox(TreeView *owner, bool opening);

	// should the bitmap data be freed?, defaults to false
	bool			freeBitmap;
};

#endif
