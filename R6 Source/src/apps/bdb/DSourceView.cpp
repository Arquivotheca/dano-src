/*	$Id: DSourceView.cpp,v 1.13 1999/05/11 21:31:05 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/12/98 19:40:07
*/

#include "bdb.h"
#include "DSourceView.h"
#include "DTeam.h"
#include "DThread.h"
#include "DSymWorld.h"
#include "DMessages.h"
#include "CAlloca.h"
#include "DSourceCode.h"
#include "DStatement.h"
#include "DNub.h"
#include "DThreadWindow.h"
#include "DCpuState.h"

#include <Messenger.h>
#include <Bitmap.h>
#include <FilePanel.h>
#include <Path.h>
#include <StringView.h>
#include <File.h>
#include <Roster.h>
#include <ScrollBar.h>

static uchar *kPCIcon = NULL, *kBPIcon = NULL, *kAddrIcon;
BFilePanel *DSourceView::sfLocatePanel;

const float kHOffset = 30;

DSourceView::DSourceView(BRect frame, const char *name, DTeam& team, BStringView *label)
	: BView(frame, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS)
	, fTeam(team)
	, fLabel(label)
{
	if (kPCIcon == NULL)
	{
		kPCIcon = (uchar *)HResources::GetResource('MICN', 1000);
		FailNilRes(kPCIcon);
		kBPIcon = (uchar *)HResources::GetResource('MICN', 1001);
		FailNilRes(kBPIcon);
		kAddrIcon = (uchar *)HResources::GetResource('MICN', 1002);
		FailNilRes(kAddrIcon);
	}
	
	fFile = -1;
	
	BRect r(Bounds());
	r.left = kHOffset;
	AddChild(fCode = new DSourceCode(r, "code view"));

	font_height fh;
	fCode->GetFontHeight(&fh);
	fLineHeight = ceil(fh.ascent + fh.descent + fh.leading);
	fDescent = fh.descent;
} /* DSourceView::DSourceView */

DSourceView::~DSourceView()
{
} /* DSourceView::~DSourceView */

void DSourceView::DrawLine(int lineNr)
{
	BPoint p;
	
	BRect r(0, (lineNr - 1) * fLineHeight + 2, 12, lineNr * fLineHeight);
	FillRect(r, B_SOLID_LOW);
	
	r.OffsetBy(14, 0);
	FillRect(r, B_SOLID_LOW);
	
	if (lineNr == fPCLine)
	{
		BBitmap bm(BRect(0, 0, 15, 8), B_COLOR_8_BIT);
		
		if (fKind == bpShowAddr)
			bm.SetBits(kAddrIcon, 144, 0, B_COLOR_8_BIT);
		else if (fKind == bpCurrent)
			bm.SetBits(kPCIcon, 144, 0, B_COLOR_8_BIT);
		else
		{
			uchar icon[144];
			for (int i = 0; i < 144; i++)
				icon[i] = gDisabledMap[kPCIcon[i]];
			bm.SetBits(icon, 144, 0, B_COLOR_8_BIT);
		}
		
		p.Set(16, lineNr * fLineHeight - fDescent - 7);
		
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(&bm, p);
		SetDrawingMode(B_OP_COPY);
	}
	
	p.Set(4, lineNr * fLineHeight - fDescent);

	if (fBreakpoints.count(lineNr))
		DBreakpoint::Draw(this, p, ebAlways);
	else if (fStops.count(lineNr))
		DBreakpoint::Draw(this, p, ebDisabled);
} /* DSourceView::DrawLine */

void DSourceView::Draw(BRect update)
{
	FillRect(update, B_SOLID_LOW);
	
	SetHighColor(kShadow);
	StrokeLine(BPoint(13, update.top), BPoint(13, update.bottom));
	SetHighColor(kBlack);
	
	int x1, x2, x;
	
	x1 = std::max((int)floor(update.top / fLineHeight) - 1, 0);
	x2 = (int)ceil(update.bottom / fLineHeight) + 1;
	
	for (x = x1; x <= x2; x++)
		DrawLine(x);
} /* DSourceView::Draw */

void DSourceView::SetSourceFile(DFileNr file)
{
	BAutolock lock(fTeam);

	try
	{
		if (lock.IsLocked() && fFile != file)
		{
			fFile = file;
	
			if (! DSourceFileTable::Instance().Located(file) &&
				! DSourceFileTable::Instance().Ignore(file) &&
				(sfLocatePanel == NULL || !sfLocatePanel->IsShowing()))
			{
				BMessage *msg = new BMessage(kMsgFileLocated);
				msg->AddInt32("file", file);
				
				if (sfLocatePanel == NULL)
				{
					sfLocatePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
						B_FILE_NODE, false, msg, NULL, false, true);
					FailNil(sfLocatePanel);
				}
				else
					sfLocatePanel->SetMessage(msg);
				
				string t = "Please locate: ";
				t += DSourceFileTable::Instance().Basename(file);
				
				sfLocatePanel->Window()->SetTitle(t.c_str());
				sfLocatePanel->Show();
			}
			
			if (fLabel)
			{
				BEntry e(&DSourceFileTable::Instance()[file]);
				if (e.Exists())
					fLabel->SetText(BPath(&e).Path());
				else
					fLabel->SetText("");
			}
	
			BFile f;
			(void)f.SetTo(&DSourceFileTable::Instance()[file], B_READ_ONLY);
			
			if (f.InitCheck() == B_OK)
			{
				fCode->NewFile(f);
				ScrollBar(B_VERTICAL)->SetRange(0, fCode->LineCount() * fLineHeight - Bounds().Height());
				ScrollBar(B_VERTICAL)->SetSteps(fLineHeight, fLineHeight * (floor(Bounds().Height() / fLineHeight) - 1));
				
				fTeam.GetStopsForFile(fFile, fStops, fBreakpoints);
				
				fPCLine = -1;
			}
			else
			{
				if (sfLocatePanel == NULL || !sfLocatePanel->IsShowing() && ! DSourceFileTable::Instance().Ignore(file))
					HErr("File %s not found!", DSourceFileTable::Instance()[file].name).DoError();
				Clear();
			}

			// we've either set up a new source file, or cleared the current one because we don't have
			// source available, so now we induce a redraw to flush the proper display to the window.
			Invalidate();
			fCode->Invalidate();
		}
	}
	catch (HErr& e)
	{
		e.DoError();
		Clear();
	}
} /* DSourceView::SetSourceFile */

void DSourceView::SetStatement(DStatement& statement, int kind)
{
	if (fFile != statement.fFile)
		SetSourceFile(statement.fFile);
	
	if (fPCLine != statement.fLine || kind != fKind)
	{
		fKind = kind;
		
		int pcline = fPCLine;
		fPCLine = statement.fLine;
		
		if (pcline >= 0)
			DrawLine(pcline);

		if (fPCLine >= 0)
			DrawLine(fPCLine);
	}
	
//	ScrollToPC();
	CenterPC();
} /* DSourceView::SetStatement */

void DSourceView::FrameResized(float nWidth, float nHeight)
{
	BView::FrameResized(nWidth, nHeight);
	float rmax = std::max((float)0, fCode->LineCount() * fLineHeight - nHeight);
	ScrollBar(B_VERTICAL)->SetRange(0, rmax);
	ScrollBar(B_VERTICAL)->SetSteps(fLineHeight, fLineHeight * (floor(nHeight / fLineHeight) - 1));
} /* DSourceView::FrameResized */

void DSourceView::ScrollToPC()
{
	float y;
	
	y = (fPCLine - 1) * fLineHeight;
	
	if (y < Bounds().top)
	{
		ScrollBar(B_VERTICAL)->SetValue((fPCLine - 1) * fLineHeight);
		return;
	}
	
	y = (fPCLine + 1) * fLineHeight;
	
	if (y > Bounds().bottom)
	{
		int tl = (int)(fPCLine - (Bounds().Height() / fLineHeight));
		ScrollBar(B_VERTICAL)->SetValue(tl * fLineHeight);
	}
} /* DSourceView::ScrollToPC */

void DSourceView::CenterPC()
{
	float y;
	
	y = (fPCLine - 1) * fLineHeight;
	
	if (y < Bounds().top || y + fLineHeight > Bounds().bottom)
	{
		int tl = (int)(fPCLine - (Bounds().Height() / fLineHeight) / 2);
		ScrollBar(B_VERTICAL)->SetValue(tl * fLineHeight);
	}
} /* DSourceView::CenterPC */

void DSourceView::Clear()
{
	fStops.clear();
	fBreakpoints.clear();
	
	fFile = -1;
	fPCLine = -1;

	fCode->Clear();
	Invalidate();
} /* DSourceView::Clear */

void DSourceView::MouseDown(BPoint where)
{
	int lineNr = -1;

	try
	{
		if (where.x >= 0 && where.x <= 13)
		{
			lineNr = (int)((where.y / fLineHeight) + 1);
			if (fStops.find(lineNr) == fStops.end())
				return;
			
//			if (lineNr == fPCLine)
//				return;
	
			ptr_t pc = fTeam.GetSymWorld().GetStatementOffset(fFile, lineNr);
			if (pc == 0)
				THROW(("The breakpoint at line nr %d is not valid", lineNr));
			
			bool isSet = fBreakpoints.count(lineNr);
			
			BPoint p(4, lineNr * fLineHeight - fDescent);
			
			if (DBreakpoint::Track(this, p, isSet))
			{
				BAutolock lock(fTeam);
				
				if (lock.IsLocked())
				{
					if (isSet)
					{
						fTeam.ClearBreakpoint(pc);
						fBreakpoints.erase(lineNr);
					}
					else
					{
						int32 modifiers;
						Looper()->CurrentMessage()->FindInt32("modifiers", &modifiers);
						
						DThreadWindow *w = dynamic_cast<DThreadWindow*>(Window());
						thread_id tid = (w && modifiers & B_OPTION_KEY) ? w->GetThread().GetID() : -1;
						
						fTeam.SetBreakpoint(pc, tid);
						fBreakpoints.insert(lineNr);
						
						if (tid >= 0)
							Window()->PostMessage(kMsgRun);
					}
				}
			}
		}
		else if ((int)(where.y / fLineHeight) + 1 == fPCLine &&
			fKind == bpCurrent &&
			dynamic_cast<DThreadWindow*>(Window()) != NULL)
		{
			ptr_t pc = fTeam.GetSymWorld().GetStatementOffset(fFile, fPCLine);
			
			ptr_t low, high;
			low = fTeam.GetSymWorld().GetFunctionLowPC(pc);
			high = fTeam.GetSymWorld().GetFunctionHighPC(pc);
			
			DStatement sl, sh;
			fTeam.GetSymWorld().GetStatement(low, sl);
			fTeam.GetSymWorld().GetStatement(high - 1, sh);
			
			if (TrackPC(sl.fLine, sh.fLine, where))
			{
				pc = fTeam.GetSymWorld().GetStatementOffset(fFile, fPCLine);
				DThread& t = dynamic_cast<DThreadWindow*>(Window())->GetThread();
				t.GetCPU().SetPC(pc);
			}
		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}

	if (lineNr >= 0)
		DrawLine(lineNr);
} /* DSourceView::MouseDown */

void DSourceView::NoPC()
{
	Invalidate();
} /* DSourceView::NoPC */

void DSourceView::ReloadBreakpoints()
{
	BAutolock lock(fTeam);
	
	if (lock.IsLocked())
	{
		fTeam.GetStopsForFile(fFile, fStops, fBreakpoints);
		Invalidate();
	}
} // DSourceView::ReloadBreakpoints

bool DSourceView::TrackPC(int lowLine, int highLine, BPoint p)
{
	int pLine = fPCLine;
	uint32 btns;
	
	while (GetMouse(&p, &btns), btns)
	{
		int nLine;
		
		if (p.x < 0 || p.x > kHOffset + 10)
			nLine = pLine;
		else
			nLine = (int)(p.y / fLineHeight) + 1;
		
		if (nLine < lowLine) nLine = lowLine;
		if (nLine > highLine) nLine = highLine;
		
		if (fPCLine != nLine && fStops.count(nLine))
		{
			std::swap(nLine, fPCLine);

			DrawLine(nLine);
			DrawLine(fPCLine);
			
			ScrollToPC();
		}
	}
	
	return fPCLine != pLine;
} // DSourceView::TrackPC

void DSourceView::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgFileLocated:
		{
			entry_ref ref;
			DFileNr fileNr;
			
			FailOSErr(msg->FindInt32("file", (int32*)&fileNr));
			FailOSErr(msg->FindRef("refs", &ref));
			
			DSourceFileTable::Instance().LocatedFile(fileNr, ref);
			BMessenger(fTeam.GetOwner()).SendMessage(kMsgFilesChanged);
			
			DStatement stmt;
			stmt.fFile = fileNr;
			stmt.fLine = fPCLine;
			
			fFile = -1;
			SetStatement(stmt, fKind);
			break;
		}
		
		case kMsgFileOpenInPreferredEditor:
			if (fFile >= 0) {
				entry_ref ref(DSourceFileTable::Instance()[fFile]);
				BRoster().Launch(&ref);
			}
			break;

		case B_CANCEL:
		{
			DFileNr fileNr;
			
			FailOSErr(msg->FindInt32("file", (int32*)&fileNr));
			DSourceFileTable::Instance().IgnoreFile(fileNr);
			break;
		}
		
		default:
			BView::MessageReceived(msg);
	}
} // DSourceView::MessageReceived
