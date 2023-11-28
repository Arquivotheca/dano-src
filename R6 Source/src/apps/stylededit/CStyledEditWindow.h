// ============================================================
//  CStyledEditWindow.h	©1996 Hiroshi Lockheimer
// ============================================================


#include <Window.h>


const ulong	msg_NewWindow 	= 'NewW';
const ulong	msg_OpenWindow	= 'OpnW';
const ulong	msg_GotoLine		= 'GotL';


class BFile;
class BFilePanel;
class BMenu;
class CTextView;
class BEntry;


class CStyledEditWindow : public BWindow {
public:
						CStyledEditWindow(entry_ref *inParent);
						CStyledEditWindow(entry_ref *inRef, entry_ref *inParent, uint32 encoding);
	virtual				~CStyledEditWindow();
						
	virtual void		MessageReceived(BMessage *inMessage);
	virtual bool		QuitRequested();
	virtual	void		MenusBeginning();
	virtual void		WindowActivated(bool state);
	
	void				SetDirty(bool inDirty);
	
	void				ReplaceSame();

	static long					WindowCount();
	static const BList*			WindowList();
	static CStyledEditWindow*	FindWindow(entry_ref *inRef);
	static uint32				EncodingSettingOfFilePanel(BFilePanel *panel);

	
protected:
	void				InitWindow(entry_ref *parent);
	
	void				Find();
	void				FindAgain();
	void				FindSelection();
	void				Replace();

	void				FontSelected(const font_family family, 
									 const font_style style);
	void				SizeSelected(float size);
	void				ColorSelected(rgb_color color, bool penColor);
	void				SetWrapping(bool wrap);

	void				SaveFile(entry_ref *directory, const char *name);
	void				Save();
	void				SaveAs();
	void				Revert();
	
	void				WriteData();
	void				ReadData();

	status_t			PageSetup();
	void				Print();

	void				SetReadOnly();

	static void			AddEncodingMenuToFilePanel(BFilePanel *panel, uint32 encoding);

	static BRect		GetFrameRect();
					
private:
	BEntry*				mEntry;
	BEntry*				mDirEntry;
	BFilePanel*			mOpenPanel;
	BFilePanel*			mSavePanel;
	bool				mDirty;
	bool				mWaitForSave;
	bool				m_is_source;
	CTextView*			mTextView;
	BMenu*				mFileMenu;
	BMenu*				mEditMenu;
	BMenu*				mSizeMenu;
	BMenu*				mColorMenu;
	BMenu*				mFontMenu;
	BMenu*				mDocumentMenu;
	BMessage*			mPrintSettings;
	uint32				mEncoding;
		
	static BList		sWindowList;
	static long			sUntitledCount;
};
