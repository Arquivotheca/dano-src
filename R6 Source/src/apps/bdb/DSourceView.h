/*	$Id: DSourceView.h,v 1.9 1999/05/11 21:31:05 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/12/98 19:30:29
*/

#ifndef DSOURCEVIEW_H
#define DSOURCEVIEW_H

#include "DSourceFileTable.h"

#include <set>

class BStringView;
class BFilePanel;
class DCpuState;
class DTeam;
class DSourceCode;
class DStatement;

enum
{
	bpNotCurrent,
	bpCurrent,
	bpShowAddr
};

class DSourceView : public BView
{
  public:
	DSourceView(BRect frame, const char *name, DTeam& team, BStringView *label = NULL);
	virtual ~DSourceView();
		
	virtual void Draw(BRect update);
	virtual void MouseDown(BPoint where);
	virtual void MessageReceived(BMessage *msg);
	
	virtual void SetSourceFile(DFileNr nr);
	virtual void SetStatement(DStatement& statement, int kind);
	virtual void Clear();
	virtual void NoPC();
	
	void ReloadBreakpoints();
	
  protected:
		
	virtual void DrawLine(int lineNr);
	virtual void ScrollToPC();
	virtual void CenterPC();
	
	bool TrackBreakpoint(int lineNr, BPoint p, bool wasSet);
	bool TrackPC(int lowLine, int highLine, BPoint p);

	virtual void FrameResized(float nWidth, float nHeight);
	
	DFileNr fFile;
	int fPCLine;
	DTeam& fTeam;
	float fLineHeight, fDescent;
	int fKind;
	DSourceCode *fCode;
	std::set<int> fStops, fBreakpoints;
	BStringView *fLabel;
	static BFilePanel *sfLocatePanel;
};

#endif
