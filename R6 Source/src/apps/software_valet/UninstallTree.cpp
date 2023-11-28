#include <string.h>
#include "UninstallTree.h"
#include "DoIcons.h"
#include "MyDebug.h"
#include <Looper.h>
#include <ScrollBar.h>


static TreeItem* CreateItem(bool folder = FALSE);

UninstallTree::UninstallTree(BRect r)
	:	TreeView(r,"uninsttree",B_EMPTY_STRING,
				new BMessage('????'),B_FOLLOW_ALL)
{	
	#if 0
		AddFilePath("mk/AppSketcher/Application");
		AddFilePath("mk/AppSketcher/ReadMe");
		AddFilePath("mk/AppSketcher/Data/index.html");
		AddFilePath("mk/system/lib/lrlib.so");
		AddFilePath("mk/AppSketcher/Data/dorf.html");
		AddFilePath("mk/AppSketcher/Data/spam.html");
		AddFilePath("mk/AppSketcher/Docs/You're a fool!");
	#endif
}

void UninstallTree::CollapseBig(TreeItem *parent)
{
	int childlessChildren = 0;
	TreeItem *item = parent->Children();
	while(item) {
		if (item->HasChildren()) {
			CollapseBig(item);
		}
		else {
			childlessChildren++;
		}
		item = item->Sibling();
	}
	if (childlessChildren > 5)
		parent->Collapse();
}

void UninstallTree::AddFilePath(const char *path, BBitmap *sicon)
{
	char buf[B_FILE_NAME_LENGTH];
	const char *s;
	char *d;
	TreeItem *parent = Root();
	TreeItem *tItem;
	s = path;
	while (*s) {	
		tItem = parent->Children();
		d = buf;
		// copy path component info tempbuf
		while(*s && *s != '/') {
			*d++ = *s++;
		}
		*d = 0;
		
		PRINT(("BUF IS %s\n",buf));
		
		// either null or / was encountered
		// skip over slash if not at end
		if (*s) {
			*s++;
		}
		if (!*buf)
			continue;
		// now loop to find the child
		while (tItem) {
			if (strcmp(tItem->Label(),buf) == 0) {
				break;
			}
			tItem = tItem->Sibling();
		}
		if (!tItem) {
			// wasn't found
			if (!*s)
				tItem = new TFileItem(buf, sicon);
			else
				tItem = new TFolderItem(buf);
			parent->AddChild(tItem);
		}
		parent = tItem;
	}
}

void UninstallTree::MouseDown(BPoint where)
{
	BMessage *msg = Looper()->CurrentMessage();

	ulong mods = msg->FindInt32("modifiers");
	msg->ReplaceInt32("modifiers",mods | B_COMMAND_KEY);
	
	TreeView::MouseDown(where);
}


void UninstallTree::KeyDown(const char *bytes, int32 numBytes)
{
	BScrollBar *v = ScrollBar(B_VERTICAL);
	
	switch (*bytes) {
		case B_DOWN_ARROW:
		{
			if (v)
				v->SetValue(v->Value() + 19);
			break;
		}
		case B_UP_ARROW:
		{
			if (v)
				v->SetValue(v->Value() - 19);
			break;
		}
		case B_PAGE_DOWN:
		{
			if (v)
				v->SetValue(v->Value() + Bounds().Height() - 19);
			break;
		}
		case B_PAGE_UP:
		{
			if (v)
				v->SetValue(v->Value() - Bounds().Height() +19);
			break;
		}
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		{
			BScrollBar *h = ScrollBar(B_HORIZONTAL);
			if (h) {
				if (*bytes == B_LEFT_ARROW)
					h->SetValue(h->Value() - 19);
				else
					h->SetValue(h->Value() + 19);
			}
			break;
		}
		default:
		{
			TreeView::KeyDown(bytes,numBytes);
			break;
		}
	}
}

void UninstallTree::MessageReceived(BMessage *m)
{
	if (m->what == T_SELECT_ALL)
		SelectAll(TRUE);
	else
		TreeView::MessageReceived(m);
}


static TreeItem* CreateItem(bool folder)
{
	static int createcount = 0;
	char buff[50];
	
	sprintf( buff, "Item%d", ++createcount);
	
	if (folder)
		return new TFolderItem(buff);
	else
		return new TFileItem(buff,gGenericFileSIcon);
}

TFolderItem::TFolderItem(const char *name)
	:	CTreeItem(name, gGenericFolderSIcon)
{
}

void TFolderItem::UserSelect( bool select )
{
	CTreeItem::Select(select);
	
	SetWasSelected(select);
	TreeItem* here = Children();
	while( here)
	{
		here->UserSelect(select);
		here->SetWasSelected(select);
		here = here->Sibling();
	}
}


TFileItem::TFileItem(const char *name, BBitmap *sicon)
	:	CTreeItem(name, sicon)
{
	freeBitmap = true;
}
