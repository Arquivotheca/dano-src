#include <Looper.h>
#include <ScrollBar.h>
#include "FileTree.h"
#include "FSIcons.h"
#include "PTreeItem.h"

#include "MyDebug.h"


FileTree::FileTree(BRect r)
	:	TreeView(r,"filetree",B_EMPTY_STRING,
				new BMessage('????'),B_FOLLOW_ALL)
{
	//record_ref ref;
	//get_ref_for_path("/mk/AppSketcher",&ref);
	
	//CTreeItem *first = new TFolderItem(ref);
	//first->CollapseTo(1,16);
	//AddItem(first);
	
	SelectAll(TRUE);
}


void FileTree::MouseDown(BPoint where)
{
	BMessage *msg = Looper()->CurrentMessage();

	ulong mods = msg->FindInt32("modifiers");
	msg->ReplaceInt32("modifiers",mods | B_COMMAND_KEY);
	
	TreeView::MouseDown(where);
}


void FileTree::KeyDown(const char *bytes, int32 numBytes)
{
	
	BScrollBar *v = ScrollBar(B_VERTICAL);
	
	switch (*bytes) {
		case B_DOWN_ARROW:
			if (v)
				v->SetValue(v->Value() + 19);
			break;
		case B_UP_ARROW:
			if (v)
				v->SetValue(v->Value() - 19);
			break;
		case B_PAGE_DOWN:
			if (v)
				v->SetValue(v->Value() + Bounds().Height() - 19);
			break;
		case B_PAGE_UP:
			if (v)
				v->SetValue(v->Value() - Bounds().Height() +19);
			break;
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
			FileTree::KeyDown(bytes, numBytes);
			break;
	}
}

void FileTree::MessageReceived(BMessage *m)
{
	if (m->what == T_SELECT_ALL)
		SelectAll(TRUE);
	else
		FileTree::MessageReceived(m);
}

TreeItem *FileTree::AddFilePath(char *path)
{
	path;
	return NULL;

}
