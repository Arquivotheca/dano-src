/*	$Id: DSourceCode.h,v 1.6 1999/01/05 22:09:19 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/27/98 15:13:25
*/

#ifndef DSOURCECODE_H
#define DSOURCECODE_H

#include <View.h>

#include <vector>
#include <string>

using std::string;

class DSourceCode : public BView {
public:
			DSourceCode(BRect frame, const char *name);
			
virtual	void MessageReceived(BMessage *msg);			
virtual	void Draw(BRect update);
			
			void NewFile(BPositionIO& file);
			void Clear();
			
			int LineCount() const;

private:
virtual	void AttachedToWindow();
			void DrawLine(int lineNr);	
			void WrapText();
			int Offset2Line(int offset);
			
			void ColorLine(const char *text, int size, int&state, int *starts, rgb_color *colors);
			
			void ShowFunctionMenu(BPoint where);
			void ScanForFunctions(BList& list);
			void JumpToFunction(BMessage *msg);
			
			void MouseDown (BPoint where);
			BPoint Offset2Position(int offset);
			int LinePosition2Offset(int line, float x);
			int Position2Offset(BPoint where);
			void ChangeSelection(int from, int to);
			void Select(int from, int to);
			void Selection2Region(BRegion& rgn);
			void TouchLines(int from, int to);
			void RedrawDirtyLines();
			void MakeFocus(bool);
			void Copy();
			bool WaitMouseMoved(BPoint where);
			
			void Find(bool ignoreCase, bool fromTop, bool backwards = false);
			void FindAgain(bool backwards);

// syntax colouring support routines			
static		int Move(int ch, int state);
static		int IsKeyWord(int state);

// syntax colouring tables
static		uchar *ec;
static		ushort *accept, *base, *nxt, *chk;
			
			int fAnchor, fCaret;
			BFont fFont;
			float fLineHeight, fTabWidth;
			font_height fFH;
			std::vector<int> fLineBreaks, fLineStates;
			std::vector<bool> fDirty;
			char *fText;
			int fTextSize;
			static string sfSearchText;
			static bool sfIgnoreCase;
};

inline int DSourceCode::LineCount() const
{
	return fLineBreaks.size();
} /* DSourceCode::LineCount */

#endif
