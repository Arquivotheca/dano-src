#ifndef _UNINSTALLTREE_H_
#define _UNINSTALLTREE_H_


// UninstallTree.h
#include "TreeView.h"
#include "CTreeItem.h"
#include <Rect.h>
#include <Point.h>
#include <Message.h>
#include <Bitmap.h>

enum {
	T_SELECT_ALL	= 'SelA'

};


class UninstallTree : public TreeView
{
public:
	UninstallTree(BRect frame);
	
	virtual void	MouseDown(BPoint where);
	virtual void	KeyDown(const char *bytes, int32 numBytes);
	virtual void	MessageReceived(BMessage *);
	
			void	AddFilePath(const char *path, BBitmap *icon);
			void	CollapseBig(TreeItem *parent);
};

// folder tree item
class TFolderItem : public CTreeItem
{
public:
	TFolderItem(const char *name);
	//TFolderItem(record_ref ref);
	
	virtual void	UserSelect(bool);
	
//	virtual void	DrawPlusBox(TreeView *owner);
//	virtual void	DrawMinusBox(TreeView *owner);
	
//	virtual void	AnimateBox(TreeView *owner, bool opening);
};

// file tree item
class TFileItem : public CTreeItem
{
public:
	TFileItem(const char *name, BBitmap *bmap);
	// TFileItem(BEntry *entry);
	//TFileItem(record_ref ref);
};


#endif
