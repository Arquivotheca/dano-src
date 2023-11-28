#ifndef _FILETREE_H_
#define _FILETREE_H_

#include <Rect.h>
#include <Message.h>
#include <Point.h>
#include "TreeView.h"

enum {
	T_SELECT_ALL	= 'SelA'

};


class FileTree : public TreeView
{
public:
	FileTree(BRect frame);
	
	virtual void	MouseDown(BPoint where);
	virtual void	KeyDown(const char *bytes, int32 numBytes);
	virtual void	MessageReceived(BMessage *);
	
	TreeItem		*AddFilePath(char *path);
};

#endif
