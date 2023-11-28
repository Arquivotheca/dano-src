/* $Id: XView.h,v 1.2 1998/10/21 12:51:46 maarten Exp $

	Copyright 1997, Hekkelman Programmatuur

*/

const ulong
	msg_ChangeReplace 	= 'Rplc',
	msg_Undo			= 'Undo';

class DTeam;

class XView : public BView
{
  public:
	XView(BRect frame, DTeam& team, ptr_t addr, size_t size);
	~XView();
	
	void Draw(BRect updateRect);
	void MouseDown(BPoint where);
	void MouseMoved(BPoint where, uint32 code, const BMessage *a_message);
	void KeyDown(const char *bytes, int32 numBytes);
	void FrameResized(float width, float height);
	void MessageReceived(BMessage *msg);
	void WindowActivated(bool active);

	void SetScrollBar(BScrollBar *scrollBar);
	
	void Select(int from, int to);
	void SelectAll();
	
	void SaveData();
	bool IsDirty();
	void SetDirty(bool dirty);

	void ScrollToSelection()			{ this->ScrollToCaret(); }
	
	ptr_t GetSelectionAddress();
	ptr_t GetSelectionValue();
	
	void RefreshView();
	void RefreshViewAt(ptr_t addr);
	
  private:

	void InitializeMemoryBuffer(ptr_t addr, size_t size);
	void DoMemoryRead(ptr_t addr);
	void ScrollToCaret();
	void DoCopy();
	void DoPaste();

	void Step(int key, bool extend);
	
	void Pulse();
	void ToggleCaret();
	inline void HideCaret();
	inline void ShowCaret();
	
	void DrawLine(int lineNr);
	void DrawAllLines(int lineNr);
	
	void GetSelection(BRegion& rgn, bool hex);
	void InvertSelection(bool hex);
	void ChangeSelection(int newCaret);
	void Pt2Pos(BPoint where, int& pos, bool& inHex);
	void CharRect(int pos, bool hex, BRect& r);
	
//	void StoreForUndo(int key);
//	void UndoRestart();
//	void Undo();
	
	bigtime_t fCaretTime;
	BScrollBar *fScrollBar;
	char *fData;
	ptr_t fAddr;
	int fDataSize, fLines;
	font_height fFH;
	float fCharWidth, fLineHeight;
	int fCaret, fAnchor;
	DTeam& fTeam;
//	char *fUndoBuffer;
//	int fUndoStart, fNewStart, fNewEnd;
//	int fUndoState;
	bool fEditHex;
	bool fShowCaret;
	bool fUpperNibble;
	bool fDirty;
	bool fActive;
};

inline void XView::HideCaret()
{
	if (fShowCaret)
		ToggleCaret();
} /* XView::HideCaret */

inline void XView::ShowCaret()
{
	if (!fShowCaret)
		ToggleCaret();
} /* XView::HideCaret */
