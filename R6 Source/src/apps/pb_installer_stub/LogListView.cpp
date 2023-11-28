#include	"LogListView.h"
#include	"LogEntry.h"

#include	"Log.h"

LogListView::LogListView(BRect r)
	:	BListView(r, "listview",B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL)
{	
	SetFontSize(34.0);
}

LogListView::~LogListView()
{
	int32 n = CountItems();
	for (int i = 0; i < n; i++) {
		delete(ItemAt(i));
	}
}

void LogListView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	Log log;
	
	while(true) {
		BMessage *m = log.GetNextEntry();
		if (m == NULL) break;
		LogEntry *entry = new LogEntry();
		entry->SetEntry(m);
		
		if (entry->DisplayOrNot(entry->entry->what)) {
			AddItem(entry);
		}
		else
			delete entry;
		
		//BFont	f(be_plain_font);
		//f.SetSize(36.0);
		
		//entry->SetHeight(50.0);
		//entry->Update(this,&f);
	}
	SetViewColor(230, 230, 230);
	//log.EraseLogFile();	
	
	
	MakeFocus(true);
	int32 nItems = CountItems();
	Select(nItems-1);
	ScrollToSelection();
	
	FrameResized(Bounds().Width(),Bounds().Height());
}

