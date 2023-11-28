/*	$Id: DBreakpoint.cpp,v 1.9 1999/05/11 21:31:03 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman

*/

#include "bdb.h"
#include "DBreakpoint.h"
#include "DTeam.h"
#include "DFunction.h"
#include "DSymWorld.h"
#include "DStatement.h"

#include <Path.h>
#include <Bitmap.h>
#include <devel/Unmangle.h>

unsigned char *DBreakpoint::sfIcon = NULL;

BString DBreakpoint::kAlwaysTrue("");

void UnmangleInto(const string& raw, string& demangledOut)
{
	char demangledName[UNAME_SIZE];
	if (demangle(raw.c_str(), demangledName, UNAME_SIZE) > 0) {
		demangledOut = demangledName;
	}
	else {
		demangledOut = raw;
	}
}

DBreakpoint::DBreakpoint()
			: fCondition(kAlwaysTrue), fSkipCount(0), fHits(0)
{
	fPC = 0;
	fLine = 0;
} // DBreakpoint::DBreakpoint

DBreakpoint::DBreakpoint(DTeam& team, DFileNr file, int line, ptr_t pc, bpKind kind, bool sourceLevel, thread_id thread)
			: fCondition(kAlwaysTrue)
{
	fFile = file;
	fLine = line;
	fPC = pc;
	fKind = kind;
	fSourceLevel = sourceLevel;
	fThreadID = thread;
	
	team.GetSymWorld().GetRawFunctionName(pc, fRawFunction);
	UnmangleInto(fRawFunction, fFunction);
} // DBreakpoint::DBreakpoint

DBreakpoint::DBreakpoint(const DBreakpoint& bp)
			: fCondition(bp.fCondition), fSkipCount(0), fHits(0)
{
	fFile = bp.fFile;
	fLine = bp.fLine;
	fPC = bp.fPC;
	fKind = bp.fKind;
	fFunction = bp.fFunction;
	fRawFunction = bp.fRawFunction;
	fSourceLevel = bp.fSourceLevel;
	fThreadID = bp.fThreadID;
} // DBreakpoint::DBreakpoint

DBreakpoint::DBreakpoint(const char *s)
			: fCondition(kAlwaysTrue), fSkipCount(0), fHits(0)
{
	fLine = 0;
	fPC = 0;
	fKind = ebSaved;
	fThreadID = 0;
	
	char *t = strchr(s, ';'), *p;
	FailNil(t);
	
	fFunction.assign(s, (const char *)t);
	fLine = strtoul(t + 1, &t, 10);
	
	string path;
	
	p = t + 1;
	t = strchr(p, ';');
	if (t) {
		path.assign(p, t);
	}
	else {
		path = p;
	}
	
	entry_ref ref;
	FailOSErr(get_ref_for_path(path.c_str(), &ref));
	fFile = DSourceFileTable::Instance().AddFile(ref);
	if (t && strtoul(t + 1, &t, 10) == 0) {
		fKind = ebSavedDisabled;
	}
	
	fRawFunction = fFunction;
	
	// The source level flag has been added for maui
	// We have to deal with it not existing
	// (Since we didn't do asm level debugging before maui
	// we know we can just set it to false if it doesn't exist)
	
	fSourceLevel = true;
	if (*t != 0) {
		// bump past added ';'
		t += 1;
		if (strtoul(t, &t, 10) == 0) {
			fSourceLevel = false;
		}
	}
	
	// The condition has been added for maui
	// Again, we have to deal with it not existing
	
	if (*t != 0) {
		// bump past added ';'
		t += 1;
		fCondition = t;
	}
	
} // DBreakpoint::DBreakpoint

bool DBreakpoint::operator== (const DBreakpoint& bp) const
{
	if (fKind == bp.fKind)
	{
		if (fKind < ebSaved)
			return fPC == bp.fPC;
		else
			return fLine == bp.fLine && fFile == bp.fFile;
	}
	else
		return false;
} // DBreakpoint::operator==

ptr_t DBreakpoint::Revive(DTeam& team)
{
	bpKind k = fKind;

	try
	{
		if (fSourceLevel == false) 
		{
			// prime the pump if we are dealing in assembly
			// this will read in the temp .asm file so we can work
			// with it (notice that we use the function name rather
			// than a potentially stale pc -- this slows us down
			// but only when we need to because of an asm-level breakpoint)
			DStatement notUsed;
			team.GetSymWorld().GetStatement(fRawFunction.c_str(), notUsed);
		}
		
		fPC = team.GetSymWorld().GetStatementOffset(fFile, fLine);
		if (fPC == 0)
			THROW((0));	// not found, might be a breakpoint in an addon

		string raw;
		team.GetSymWorld().GetRawFunctionName(fPC, raw);
				
		if (raw != fRawFunction)
			THROW((1));	// breakpoint has become stale

		UnmangleInto(fRawFunction, fFunction);
		fKind = (k == ebSavedDisabled ? ebDisabled : ebAlways);
	}
	catch (HErr& e)
	{
		if (e != (int)0)
			throw;
		
		fPC = 0;
		fKind = k;
	}
	
	return fPC;
} // DBreakpoint::Revive

void DBreakpoint::Write(FILE *f)
{
	BPath p;
	entry_ref ref = DSourceFileTable::Instance()[fFile];
	BEntry(&ref).GetPath(&p);

	fprintf(f, "%s;%d;%s;%d;%d;%s\n", fRawFunction.c_str(), 
								 	  fLine, 
									  p.Path(), 
									  fKind == ebAlways || fKind == ebSaved,
									  fSourceLevel ? 1 : 0,
									  fCondition.String());
} // DBreakpoint::Write

void DBreakpoint::SetKind(bpKind kind)
{
	fKind = kind;
	if (kind >= ebSaved)
		fPC = 0;
} // DBreakpoint::SetKind

void DBreakpoint::Draw(BView *inView, BPoint pos, bpKind kind)
{
	if (sfIcon == NULL)
		FailNilRes(sfIcon = (unsigned char *)HResources::GetResource('MICN', 1001));
	
	if (kind == ebDisabled || kind == ebSavedDisabled)
	{
		inView->SetHighColor(kShadow);
		inView->StrokeLine(BPoint(pos.x, pos.y - 3), BPoint(pos.x + 4, pos.y - 3));
		inView->SetHighColor(kBlack);
	}
	else
	{
		BBitmap bm(BRect(0, 0, 4, 4), B_COLOR_8_BIT);
		if (kind >= ebSaved)
		{
			uchar icon[40];
			for (int i = 0; i < 40; i++)
				icon[i] = gDisabledMap[sfIcon[i]];
			bm.SetBits(icon, 40, 0, B_COLOR_8_BIT);
		}
		else
			bm.SetBits(sfIcon, 40, 0, B_COLOR_8_BIT);
	
		pos.y -= 5;
	
		inView->SetDrawingMode(B_OP_OVER);
		inView->DrawBitmap(&bm, pos);
		inView->SetDrawingMode(B_OP_COPY);
	}
} // DBreakpoint::Draw

bool DBreakpoint::Track(BView *inView, BPoint pos, bool wasSet)
{
	BRect r;
	BPoint where, p1, p2;
	ulong btns;
	bool in = true;
	float y = pos.y - 3;
	
	r.Set(pos.x - 1, pos.y - 6, pos.x + 6, pos.y + 1);
	inView->FillRect(r, B_SOLID_LOW);
	
	p1.x = pos.x;
	p1.y = y;
	p2.x = pos.x + 4;
	p2.y = y;
	
	inView->SetHighColor(255, 0, 0, 255);
	inView->StrokeLine(p1, p2);
	
	do
	{
		snooze(25000);
		inView->GetMouse(&where, &btns);

		if (in != r.Contains(where))
		{
			inView->FillRect(r, B_SOLID_LOW);

			if (in)
			{
				in = false;
				if (wasSet)
					Draw(inView, pos, ebAlways);
				else
					Draw(inView, pos, ebDisabled);
			}
			else
			{
				in = true;
				inView->SetHighColor(255, 0, 0, 255);
				inView->StrokeLine(p1, p2);
			}
		}
	}
	while (btns);
	
	return in;
} // DBreakpoint::Track

const BString& DBreakpoint::GetCondition() const
{
	return fCondition;
} // DBreakpoint::GetCondition

void DBreakpoint::SetCondition(const char* condition)
{
	fCondition = condition;
} // DBreakpoint::SetCondition

// Set how many times we ignore hitting the breakpoint
void 
DBreakpoint::SetSkipCount(unsigned long skipCount)
{
	// note that we do NOT reset the hit count here!
	fSkipCount = skipCount;
}

void 
DBreakpoint::SetHitCount(unsigned long hitCount)
{
	fHits = hitCount;
}

// Record that we hit the breakpoint; returns 'true' if we've skipped
// it enough times to actually stop the thread now.
bool 
DBreakpoint::Hit()
{
	fHits++;
	return (fHits >= fSkipCount);
}
