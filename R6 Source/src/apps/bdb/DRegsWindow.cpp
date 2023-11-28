/*	$Id: DRegsWindow.cpp,v 1.9 1999/05/03 13:09:51 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/08/98 14:53:40
*/

#include "bdb.h"
#include "DRegsWindow.h"
#include "DMessages.h"
#include "DMemoryWindow.h"
#include "DTeam.h"
#include "DCpuState.h"

#include <OutlineListView.h>
#include <Beep.h>

struct SRegInfo
{
	char regName[12];
	long regOffset;
	long regSize;
	long regType;
};

DRegsWindow::DRegsWindow(BRect frame, const char *name, DThread& thread, int resID)
	: BWindow(frame, name, B_FLOATING_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL, B_AVOID_FRONT | B_AVOID_FOCUS),
	  fThread(thread), fCPU(NULL)
{
	BRect b(Bounds());
	BMenuBar* mbar;
	AddChild(mbar = HResources::GetMenuBar(b, 200));
	FailNil(mbar);

	b.top = mbar->Frame().bottom;
	// don't want left border on DListBox for single pane window
	b.left -= 2;

	AddChild(fList = new DListBox(b, "myList"));
	fList->AddColumn("Name", 70);
	fList->AddColumn("Value", 40);
	
	const void *p = HResources::GetResource('RegI', resID);
	FailNilMsg(p, "Register information resources is missing");
	
	long cnt = *(long *)p;
	SRegInfo *regs = (SRegInfo *)((char *)p + sizeof(long));
	
	for (int i = 0; i < cnt; i++)
		fList->List()->AddItem(new DRegsItem(this, regs[i].regName, regs[i].regOffset, regs[i].regSize, regs[i].regType));
} /* DRegsWindow::DRegsWindow */

bool DRegsWindow::QuitRequested()
{
	Hide();
	return false;
} /* DRegsWindow::QuitRequested */

void DRegsWindow::SetTarget(BHandler *target)
{
	fTarget = target;
} /* DRegsWindow::SetTarget */

void DRegsWindow::MessageReceived(BMessage *msg)
{
	try
	{
		switch (msg->what)
		{
			case kMsgNewRegisterData:
			{
				// The DCpuState pointer we receive here is owned by the DThread, so
				// we don't touch or delete it; we just replace our private cpu state object
				// with a clone of the one we're passed
				const DCpuState* cpu;
				FailOSErr(msg->FindPointer("registers", (void **)&cpu));
				delete fCPU;
				fCPU = cpu->Clone();
				
				for (int i = 0; i < fList->List()->CountItems(); i++)
					static_cast<DRegsItem*>(fList->List()->ItemAt(i))->UpdateValue();
				
				fList->Changed();
				break;
			}
			
			case kMsgRegisterModified:
			{
				// We get this message when a DRegsItem has entered a new value
				// for some register.  It's changed our underlying CPU object directly,
				// so there's currently no CPU data in the message.  We add one and
				// then pass it along to the appropriate DThreadWindow object.
				msg->AddPointer("registers", fCPU);
				fList->Changed();
				
				FailMessageTimedOutOSErr(BMessenger(fTarget).SendMessage(msg,
					(BHandler *)0, 1000));
				break;
			}
		
			case kMsgDumpMemory:
				DumpMemory();
				break;
			
			case kMsgAddWatchpoint:
				AddWatchpoint();
				break;
			
			default:
				BWindow::MessageReceived(msg);
		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}
} // DRegsWindow::MessageReceived


ptr_t DRegsWindow::GetSelectedRegisterValue()
{
	// Get the value of the currently selected register as a pointer
	ptr_t addr[4];		// 4 ptr_t values is enough for any register
	addr[0] = 0;
	
	int index = fList->List()->FullListCurrentSelection();
	if (index >=0) 
	{
		DRegsItem* registerItem = static_cast<DRegsItem*>(fList->List()->FullListItemAt(index));
		if (registerItem)
		{
			registerItem->GetCPUValue(&addr);
		}
	}
	
	return addr[0];
} // DRegsWindow::GetRegisterValue

void DRegsWindow::DumpMemory()
{
	ptr_t addr = this->GetSelectedRegisterValue();
	// If all else fails, just show the current frame pointer memory
	if (addr == 0)
	{
		addr = fCPU->GetFP();
	}
	new DMemoryWindow(fThread.GetTeam(), addr, 4);
} // DRegsWindow::DumpMemory

void DRegsWindow::AddWatchpoint()
{
	ptr_t addr = this->GetSelectedRegisterValue();
	if (addr != 0)
	{
		fThread.GetTeam().SetWatchpoint(DTracepoint::kPlain, addr);
	}
} // DRegsWindow::AddWatchpoint

//#pragma mark -

const char* DRegsItem::GetColumnText(int column)
{
	if (column == 1)
		return fValue;
	else
		return NULL;
} // DRegsItem::GetColumnText

void DRegsItem::SetColumnText(int /*column*/, const char *newText)
{
	char *c = (char *) fWindow->RegsData().RawState();

	switch (fType)
	{
	case 0:		// integer registers are in hex
		{
			if (strncmp(newText, "0x", 2) == 0) newText += 2;

			char *t;
			unsigned long long v = strtoull(newText, &t, 16);
			if (t && *t == 0)
			{
				switch (fLength)
				{
					case 1:	*(uchar *)(c + fOffset) = v; break;
					case 2:	*(ushort *)(c + fOffset) = v; break;
					case 4:	*(ulong *)(c + fOffset) = v; break;
					case 10:
					case 16:
									*(unsigned long long*)(c + fOffset) = v; break;
					default:	ASSERT(false);
				}

				// we've modified the window's cpu state directly, so we don't
				// need to actually pass the new state object, just a notification that
				// the change has been made.
				BMessage msg(kMsgRegisterModified);
				fWindow->MessageReceived(&msg);
				
				sprintf(fValue, "%08Lx", v);
			}
			else
				beep();
			}
		break;

	case 1:		// floating-point registers are not in hex
		{
		}
		break;
	}
} // DRegsItem::SetColumnText

void DRegsItem::DrawItem(BView *owner, BRect bounds, bool /*complete*/, int column)
{
	font_height fh;
	be_plain_font->GetHeight(&fh);
	
	owner->SetLowColor(tint_color(ui_color(B_UI_DOCUMENT_BACKGROUND_COLOR),
		IsSelected() ? B_HIGHLIGHT_BACKGROUND_TINT : B_NO_TINT));
	owner->FillRect(bounds, B_SOLID_LOW);
	
	const char *s = column == 0 ? Text() : fValue;

	if (fChanged)
		owner->SetHighColor(kRed);
	
	if (column == 0)
	{
		owner->SetFont(be_plain_font);
		owner->DrawString(Text(),	BPoint(bounds.left + 2, bounds.bottom - fh.descent));
	}
	else
	{
		owner->SetFont(be_fixed_font);
		owner->DrawString(s,	BPoint(bounds.left + 2, bounds.bottom - fh.descent));
	}

	if (fChanged)
		owner->SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
} // DRegsItem::DrawItem(BView *owner, BRect bounds, bool complete, int column)

static void
write_hex(char* dest, unsigned char byte)
{
	const char digits[] = "0123456789abcdef";
	dest[0] = digits[byte >> 4];
	dest[1] = digits[byte & 0x0F];
}

void DRegsItem::UpdateValue()
{
	char sv[64];
	unsigned char v[16];		// registers are up to 16 bytes

	// write out the register value in hex, MSB first
	GetCPUValue(&v);
	memset(sv, 0, sizeof(sv));
	char* p = sv;			// destination pointer
	for (int i = fLength - 1; i >= 0; i--)		// walk MSB to LSB -- !!! little-endian specific !!!
	{
		write_hex(p, v[i]);
		p += 2;
	}

	if (fChanged = (strcmp(sv, fValue) != 0))
		strcpy(fValue, sv);
} // DRegsItem::UpdateValue

void 
DRegsItem::GetCPUValue(void* outValue)
{
	const char *c = (const char *)&fWindow->RegsData();
	memset(outValue, 0, 16);
	memcpy(outValue, c + fOffset, fLength);
} // DRegsItem::GetCPUValue
