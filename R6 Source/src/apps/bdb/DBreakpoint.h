/*	$Id: DBreakpoint.h,v 1.7 1999/05/11 21:31:03 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman

*/

#ifndef DBREAKPOINT_H
#define DBREAKPOINT_H

#include "DSourceFileTable.h"

#include <String.h>
class DTeam;
using std::string;

enum bpKind {
	ebDisabled,
	ebOneTime,
	ebAlways,
	ebSaved,
	ebSavedDisabled
};

class DBreakpoint
{
	// make this inherit from DTracepoint
  public:
  	DBreakpoint();
	DBreakpoint(DTeam& team, DFileNr file, int line, ptr_t pc, bpKind kind, bool sourceLevel, thread_id thread = 0);
	DBreakpoint(const DBreakpoint& bp);
	DBreakpoint(const char *s);

	void Write(FILE *f);
	
	ptr_t Revive (DTeam& team);
	
	DFileNr File() const							{ return fFile; }
	int Line() const								{ return fLine; }
	ptr_t PC() const							{ return fPC; }
	bpKind Kind() const						{ return fKind; }
	const char* Function () const			{ return fFunction.c_str(); }
	thread_id ThreadID() const				{ return fThreadID; }
	unsigned long SkipCount() const		{ return fSkipCount; }
	unsigned long Hits() const				{ return fHits; }
	
	void SetKind(bpKind kind);
	
	const BString& GetCondition() const;
	void SetCondition(const char* condition);

	// Setting a skip count does NOT the hit count to zero
	void SetSkipCount(unsigned long skipCount);
	void SetHitCount(unsigned long hitCount);

	// Tell the breakpoint that we've hit it; returns 'true' if we're
	// actually supposed to stop (if the hit count is greater than
	// the skip count).  Hit() does *not* evaluate the condition; that
	// should be done first, and Hit() called only when the condition
	// evaluates to 'true'.
	bool Hit();
	
	bool operator== (const DBreakpoint& bp) const;
	
	// And since breakpoints need to be drawn in several unrelated places:
	// pos is the _lower_ left corner of the breakpoint icon. Make it the same
	// as the baseline for the text following it.
	static void Draw(BView *inView, BPoint pos, bpKind kind);
	static bool Track(BView *inView, BPoint pos, bool wasSet);

	static BString kAlwaysTrue;

  private:
  	string fFunction;						// the function in which this breakpoint is set
  	string fRawFunction;
  	BString fCondition;						// if (evaluate(fCondition) == false) then continue
  	unsigned long fSkipCount;		// continue the first N types we hit this breakpoint
  	unsigned long fHits;					// how many times we've hit it so far
	DFileNr fFile;
	int fLine;	
	ptr_t fPC;
	bpKind fKind;
	thread_id fThreadID;					// the thread_id for a one time breakpoint
	bool fSourceLevel;
	static unsigned char *sfIcon;
};

#endif
