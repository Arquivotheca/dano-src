/*	$Id: DMemoryWindow.cpp,v 1.3 1998/11/17 12:16:36 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

#include "bdb.h"
#include "DMemoryWindow.h"
#include "XView.h"
#include "DTeam.h"
#include "DMessages.h"
#include "DGetMemoryLocation.h"

#include <Message.h>
#include <ScrollBar.h>
#include <Alert.h>

DMemoryWindow::DMemoryWindow(DTeam& team, ptr_t addr, size_t size)
	: BWindow(BRect(100, 100, 400, 310), "Memory 1", B_DOCUMENT_WINDOW, 0),
	  fTeam(team)
{
	BRect b(Bounds());
	BMenuBar* mbar;
	AddChild(mbar = HResources::GetMenuBar(b, 300));
	FailNil(mbar);

	BScrollBar *scrollBar;
	
	b.top = mbar->Frame().bottom;
	b.right -= B_V_SCROLL_BAR_WIDTH;
	fEditor = new XView(b, team, addr, size);

	ResizeTo(be_fixed_font->StringWidth("0") * 68 + 6 + B_V_SCROLL_BAR_WIDTH, b.Height());
	AddChild(fEditor);
	
	b = fEditor->Bounds();
	b.OffsetTo(0, 0);
	b.top = mbar->Frame().bottom;
	b.left = b.right;
	b.right += B_V_SCROLL_BAR_WIDTH;
	b.bottom -= B_H_SCROLL_BAR_HEIGHT;
	b.InsetBy(-1, -1);
	b.left += 2;
	b.top += 1;
	
	AddChild(scrollBar = new BScrollBar(b, "scrollbar", fEditor, 0, 0, B_VERTICAL));
	scrollBar->SetResizingMode(B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT);
	
	fEditor->SetScrollBar(scrollBar);
	fEditor->MakeFocus();
	
	char titleBuffer[64];
	sprintf(titleBuffer, "Memory @ 0x%lx", addr);
	DMemoryWindow::SetTitle(titleBuffer);
	Show();
	
	fAddress = addr;
} // DMemoryWindow::DMemoryWindow
	
void DMemoryWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgRefreshMemory:
			fEditor->RefreshView();
			break;

		case kMsgScrollToAnchor:
			fEditor->ScrollToSelection();
			break;

		case kMsgNewMemoryLocationCmd:
		{
			ptr_t value = fEditor->GetSelectionValue();
			DGetMemoryLocation* dialog = DialogCreator<DGetMemoryLocation>::CreateDialog(this);
			BAutolock lock(dialog);
			dialog->SetDefaultAddress(value);
			break;
		}
		
		case kMsgNewMemoryLocation:
		{
			ptr_t addr;
			FailOSErr(msg->FindInt32("address", (int32*)&addr));
			this->DoRefreshAt(addr);			
			break;
		}
		
		case kMsgPreviousMemory:
			this->DoRefreshAt(fAddress - B_PAGE_SIZE);
			break;

		case kMsgNextMemory:
			this->DoRefreshAt(fAddress + B_PAGE_SIZE);
			break;

		case kMsgAddWatchpoint:
		{
			ptr_t addr = fEditor->GetSelectionAddress();
			fTeam.SetWatchpoint(DTracepoint::kPlain, addr);
			break;
		}
		
		default:
			BWindow::MessageReceived(msg);
	}           
} // DMemoryWindow::MessageReceived

void DMemoryWindow::DoRefreshAt(ptr_t addr)
{
	try 
	{
		fEditor->RefreshViewAt(addr);
		fAddress = addr;
	}
	catch (...)
	{
		char textBuffer[16];
		sprintf(textBuffer, "%lx", addr);
		BString alertText = "Cannot read memory at address: ";
		alertText += textBuffer;
		(new BAlert("", alertText.String(), "OK"))->Go();
		fEditor->Invalidate();
	}
} // DMemoryWindow::DoRefreshAt
