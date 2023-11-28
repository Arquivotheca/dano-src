// ============================================================
//  CStyledEditWindow.cpp	�1996 Hiroshi Lockheimer
// ============================================================

#include <Debug.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Alert.h>
#include <ClassInfo.h>
#include <Clipboard.h>
#include <Directory.h>
#include <File.h>
#include <FilePanel.h>
#include <fs_attr.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <PrintJob.h>
#include <Screen.h>
#include <ScrollView.h>
#include <UTF8.h>
#include "CStyledEditWindow.h"
#include "CStyledEditApp.h"
#include "CTextView.h"
#include "CFontMenuItem.h"
#include "CColorMenuItem.h"
#include "RecentItems.h"

struct EncodingsTable {
	const char	*name;
	uint32		flavor;
};


const EncodingsTable kEncodings[] = {
	{"Central European (ISO 8859-2)", B_ISO2_CONVERSION},
	{"Cyrillic (ISO 8859-5)", B_ISO5_CONVERSION},
	{"Cyrillic (KOI8-R)", B_KOI8R_CONVERSION},
	{"Cyrillic (MS-DOS 866)", B_MS_DOS_866_CONVERSION},
	{"Cyrillic (Windows 1251)", B_MS_WINDOWS_1251_CONVERSION},
	{"Greek (ISO 8859-7)", B_ISO7_CONVERSION},
	{"Japanese (Shift-JIS)", B_SJIS_CONVERSION},
	{"Japanese (JIS)", B_JIS_CONVERSION},
	{"Japanese (EUC)", B_EUC_CONVERSION},
	{"Korean (EUC)", B_EUC_KR_CONVERSION},
	{"Unicode", B_UNICODE_CONVERSION},
	{"Unicode (UTF-8)", kUTF8Conversion},
	{"Western (Windows)", B_MS_WINDOWS_CONVERSION},
	{"Western (Macintosh)", B_MAC_ROMAN_CONVERSION}
};
const int32 kNumEncodings = sizeof(kEncodings) / sizeof(kEncodings[0]);


#ifdef SOKYO_TUBWAY
#include "BottomlineInput.h"
#endif


const ulong	msg_Save				= 'Save';
const ulong	msg_SaveAs				= 'SvAs';
const ulong msg_Revert				= 'Rvrt';
const ulong	msg_Clear				= 'Cler';
const ulong	msg_SelectAll			= 'SAll';
const ulong	msg_Find				= 'Find';
const ulong	msg_FindAgain			= 'FiAg';
const ulong	msg_FindSelection		= 'FiSl';
const ulong	msg_Replace				= 'Rplc';
const ulong	msg_ReplaceSame			= 'RpSm';
const ulong	msg_PSetup				= 'PSet';
const ulong	msg_Print				= 'Prnt';
const ulong	msg_AlignLeft			= 'ALft';
const ulong	msg_AlignCenter			= 'ACtr';
const ulong	msg_AlignRight			= 'ARgt';
const ulong	msg_SetWrap				= 'Wrap';
const ulong	msg_FamilySelected		= 'FSel';
const ulong	msg_StyleSelected 		= 'StSl';
const ulong	msg_SizeSelected 		= 'SSel';
const ulong	msg_ColorSelected		= 'CSel';
const ulong	msg_BackColorSelected	= 'BSel';

const rgb_color	kRed 		= {255, 0, 0, 255};
const rgb_color	kGreen 		= {0, 255, 0, 255};
const rgb_color	kBlue 		= {0, 0, 255, 255};
const rgb_color	kCyan 		= {0, 255, 255, 255};
const rgb_color	kMagenta 	= {255, 0, 255, 255};
const rgb_color	kYellow 	= {255, 255, 0, 255};
const rgb_color	kBlack 		= {0, 0, 0, 255};
const rgb_color	kWhite		= {255, 255, 255, 255};
const rgb_color	kGray		= {238, 238, 238, 255};
const rgb_color	kBackYellow	= {255, 255, 153, 255};
const rgb_color	kBackGreen	= {204, 255, 204, 255};
const rgb_color	kBackBlue	= {153, 255, 255, 255};
const rgb_color	kPink		= {255, 204, 204, 255};
const rgb_color	kPurple		= {204, 204, 255, 255};


#ifdef SOKYO_TUBWAY
const char*		kUntitled	= "名称未設定";
const char*		kSave1		= "閉じる前に、書類“";
const char*		kSave2		= "”の変更を保存しますか?";
const char*		kRevert1	= "書類“";
const char*		kRevert2	= "”を一つ前のバージョンに戻しますか?";
const char*		kDontSave	= "保存しない";
const char*		kCancel		= "キャンセル";
const char*		kOK			= "OK";
const char*		kFile		= "ファイル";
const char*		kNew		= "新規";
const char*		kOpen		= "開く"B_UTF8_ELLIPSIS;
const char*		kSave		= "保存";
const char*		kSaveAs		= "別名で保存"B_UTF8_ELLIPSIS;
const char*		kRevert		= "戻る";
const char*		kClose		= "閉じる";
const char*		kPSetup		= "用紙設定"B_UTF8_ELLIPSIS;
const char*		kPrint		= "印刷"B_UTF8_ELLIPSIS;
const char*		kQuit		= "終了";
const char*		kEdit		= "編集";
const char*		kUndoStrings[] = {
	"取り消し不可",
	"入力の取り消し",
	"カットの取り消し",
	"ペーストの取り消し",
	"消去の取り消し",
	"ドロップの取り消し"
};
const char*		kRedoStrings[] = {
	"やり直し不可",
	"入力のやり直し",
	"カットのやり直し",
	"ペーストのやり直し",
	"消去のやり直し",
	"ドロップのやり直し"
};
const char*		kCut		= "カット";
const char*		kCopy		= "コピー";
const char*		kPaste		= "ペースト";
const char*		kClear		= "消去";
const char*		kSelectAll	= "全てを選択";
const char*		kFind		= "検索"B_UTF8_ELLIPSIS;
const char*		kFindAgain	= "再検索";
const char*		kFindSel	= "選択範囲を検索";
const char*		kReplace	= "入れ替え"B_UTF8_ELLIPSIS;
const char*		kRSame		= "全て入れ替え";
const char*		kFont		= "フォント";
const char*		kSize		= "サイズ";
const char*		kColor		= "色";
const char*		kBlackT		= "黒";
const char*		kRedT		= "赤";
const char*		kGreenT		= "緑";
const char*		kBlueT		= "青";
const char*		kCyanT		= "シアン";
const char*		kMagentaT	= "マゼンタ";
const char*		kYellowT	= "黄色";
const char*		kDocument	= "ドキュメント";
const char*		kAlign		= "割付";
const char*		kLeft		= "左";
const char*		kCenter		= "中央";
const char*		kRight		= "右";
const char*		kWrap		= "ワードラップ";
const char*		kInfo		= "情報";
const char*		kAbout		= "SokyoTubway について";
const char*		kHelp		= "ヘルプ"B_UTF8_ELLIPSIS;
#else
const char*		kUntitled	= "Untitled";
const char*		kSave1		= "Save changes to the document "B_UTF8_OPEN_QUOTE;
const char*		kSave2		= B_UTF8_CLOSE_QUOTE"?";
const char*		kRevert1	= "Revert to the last version of "B_UTF8_OPEN_QUOTE;
const char*		kRevert2	= B_UTF8_CLOSE_QUOTE"?";
const char*		kDontSave	= "Don't Save";
const char*		kCancel		= "Cancel";
const char*		kOK			= "OK";
const char*		kFile		= "File";
const char*		kNew		= "New";
const char*		kOpen		= "Open"B_UTF8_ELLIPSIS;
const char*		kSave		= "Save";
const char*		kSaveAs		= "Save As"B_UTF8_ELLIPSIS;
const char*		kRevert		= "Revert to Saved";
const char*		kClose		= "Close";
const char*		kPSetup		= "Page Setup"B_UTF8_ELLIPSIS;
const char*		kPrint		= "Print"B_UTF8_ELLIPSIS;
const char*		kQuit		= "Quit";
const char*		kEdit		= "Edit";
const char*		kUndoStrings[] = {
	"Can't Undo",
	"Undo Typing",
	"Undo Cut",
	"Undo Paste",
	"Undo Clear",
	"Undo Drop"
};
const char*		kRedoStrings[] = {
	"Can't Redo",
	"Redo Typing",
	"Redo Cut",
	"Redo Paste",
	"Redo Clear",
	"Redo Drop"
};
const char*		kCut		= "Cut";
const char*		kCopy		= "Copy";
const char*		kPaste		= "Paste";
const char*		kClear		= "Clear";
const char*		kSelectAll	= "Select All";
const char*		kFind		= "Find"B_UTF8_ELLIPSIS;
const char*		kFindAgain	= "Find Again";
const char*		kFindSel	= "Find Selection";
const char*		kReplace	= "Replace"B_UTF8_ELLIPSIS;
const char*		kRSame		= "Replace Same";
const char*		kFont		= "Font";
const char*		kSize		= "Size";
const char*		kColor		= "Color";
const char*		kBlackT		= "Black";
const char*		kRedT		= "Red";
const char*		kGreenT		= "Green";
const char*		kBlueT		= "Blue";
const char*		kCyanT		= "Cyan";
const char*		kMagentaT	= "Magenta";
const char*		kYellowT	= "Yellow";
const char*		kDocument	= "Document";
const char*		kAlign		= "Align";
const char*		kLeft		= "Left";
const char*		kCenter		= "Center";
const char*		kRight		= "Right";
const char*		kWrap		= "Wrap Lines";
const char*		kInfo		= "Info";
const char*		kAbout		= "About StyledEdit";
const char*		kHelp		= "Help"B_UTF8_ELLIPSIS;
#endif

							
BList 	CStyledEditWindow::sWindowList;
long	CStyledEditWindow::sUntitledCount = 0;


CStyledEditWindow::CStyledEditWindow(
	entry_ref	*inParent)
	: BWindow(GetFrameRect(), B_EMPTY_STRING, B_DOCUMENT_WINDOW, 0)
{
	sUntitledCount++;
	
	InitWindow(inParent);
	
	Lock();
	
	char title[20];
	sprintf(title, "%s %ld", kUntitled, sUntitledCount);
	SetTitle(title);

	Unlock();
}


CStyledEditWindow::CStyledEditWindow(
	entry_ref	*inRef,
	entry_ref	*inParent,
	uint32		encoding)
		: BWindow(GetFrameRect(), B_EMPTY_STRING, B_DOCUMENT_WINDOW, 0)
{
	InitWindow(inParent);

	Lock();

	mEncoding = encoding;	
	mEntry = new BEntry(inRef);

	SetTitle(inRef->name);
	ReadData();

	Unlock();
}


CStyledEditWindow::~CStyledEditWindow()
{
#ifdef SOKYO_TUBWAY
	((CStyledEditApp *)be_app)->sBottomline->SetTarget(NULL);
#endif
	
	delete (mEntry);
	delete (mDirEntry);
	delete (mOpenPanel);
	delete (mSavePanel);
	delete (mPrintSettings);

	sWindowList.RemoveItem(this);
	be_app->PostMessage(msg_WindowRemoved);
}


void
CStyledEditWindow::MessageReceived(
	BMessage	*inMessage)
{
	switch (inMessage->what) {
#ifdef SOKYO_TUBWAY
		case 'KNJI':
		{
			const char *kanji = NULL;
			if (inMessage->FindString("kanji", &kanji) == B_NO_ERROR) 
				mTextView->Insert(kanji);
			break;
		}
#endif
	
		case msg_NewWindow:
		{
			entry_ref dirEntry;
			mDirEntry->GetRef(&dirEntry);
			(new CStyledEditWindow(&dirEntry))->Show();
			break;
		}

		case msg_OpenWindow:
		{
			if (mOpenPanel == NULL) {
				entry_ref dirEntry;
				mDirEntry->GetRef(&dirEntry);
				mOpenPanel = new BFilePanel(B_OPEN_PANEL, 
											&be_app_messenger, 
											&dirEntry);

				BMessage msg(B_REFS_RECEIVED);
				msg.AddPointer("stylededit_open_panel", mOpenPanel);
				mOpenPanel->SetMessage(&msg);

				AddEncodingMenuToFilePanel(mOpenPanel, mEncoding);
			}		
			mOpenPanel->Show();
			break;
		}

		case msg_Save:
			Save();
			break;
			
		case msg_SaveAs:
			SaveAs();
			break;
			
		case msg_Revert:
			Revert();
			break;
			
		case msg_Clear:
			mTextView->Clear();
			break;
			
		case msg_SelectAll:
			mTextView->SelectAll();
			break;
	
		case msg_Find:
			Find();
			break;
			
		case msg_FindAgain:
			FindAgain();
			break;
			
		case msg_FindSelection:
			FindSelection();
			break;
			
		case msg_Replace:
			Replace();
			break;
		
		case msg_ReplaceSame:
			ReplaceSame();
			break;
			
		case msg_PSetup:
			PageSetup();
			break;

		case msg_Print:
			Print();
			break;

		case msg_AlignLeft:
			if (mTextView->Alignment() != B_ALIGN_LEFT) {
				mTextView->SetAlignment(B_ALIGN_LEFT);
				SetDirty(true);
			}
			break;
			
		case msg_AlignCenter:
			if (mTextView->Alignment() != B_ALIGN_CENTER) {
				mTextView->SetAlignment(B_ALIGN_CENTER);
				SetDirty(true);
			}
			break;
			
		case msg_AlignRight: 
			if (mTextView->Alignment() != B_ALIGN_RIGHT) {
				mTextView->SetAlignment(B_ALIGN_RIGHT);
				SetDirty(true);
			}
			break;
		
		case msg_SetWrap:
		{
			BMenuItem *menu = NULL;
			inMessage->FindPointer("source", (void **)&menu);

			SetWrapping(!menu->IsMarked());

			SetDirty(true);
			break;
		}

		case msg_FamilySelected:
		{
			BMenuItem *menu = NULL;
			inMessage->FindPointer("source", (void **)&menu);
			FontSelected(menu->Label(), NULL);
			break;
		}

		case msg_StyleSelected:
		{
			BMenuItem *menu = NULL;
			inMessage->FindPointer("source", (void **)&menu);
			FontSelected(menu->Menu()->Superitem()->Label(), menu->Label());
			break;
		}
			
		case msg_SizeSelected:
		{
			BMenuItem *menu = NULL;
			inMessage->FindPointer("source", (void **)&menu);
			SizeSelected(atof(menu->Label()));
			break;
		}
			
		case msg_ColorSelected:
		case msg_BackColorSelected:
		{
			long 		dataSize = 0;
			rgb_color       *color;
			inMessage->FindData("color", B_RGB_COLOR_TYPE, (const void **)&color, &dataSize);
			ColorSelected(*color, inMessage->what == msg_ColorSelected);
			break;
		}
				
		case B_SAVE_REQUESTED:
		{
			if (mSavePanel != NULL)
				mEncoding = EncodingSettingOfFilePanel(mSavePanel);

			entry_ref dir;
			inMessage->FindRef("directory", &dir);

			const char *name = NULL;
			inMessage->FindString("name", &name);

			SaveFile(&dir, name);
			break;
		}

		case msg_GotoLine:
		{
			int32 line = 0;
			if (!inMessage->FindInt32("line", &line) && (line > 0)) {
				int32 selectionOffset = 0;
				int32 selectionLength = 0;
				inMessage->FindInt32("offset", &selectionOffset);
				inMessage->FindInt32("length", &selectionLength);
				if (line > mTextView->CountLines()) {
					line = mTextView->CountLines();
				}
				if (selectionOffset + selectionLength > 0) {
					mTextView->Select(selectionOffset, selectionOffset+selectionLength);
					mTextView->ScrollToSelection();				
				}
				else if (line > 0) {
					mTextView->Select(mTextView->OffsetAt(line-1), mTextView->OffsetAt(line));
					mTextView->ScrollToSelection();
				}
			}
			break;
		}

		case B_PRINTER_CHANGED:
			delete (mPrintSettings);
			mPrintSettings = NULL;
			break;

		default:
			BWindow::MessageReceived(inMessage);
			break;
	}
}


void
CStyledEditWindow::WindowActivated(
	bool	state)
{
#ifdef SOKYO_TUBWAY
	if (state)
		((CStyledEditApp *)be_app)->sBottomline->SetTarget(this);
#endif

	BWindow::WindowActivated(state);
}


void
CStyledEditWindow::SaveFile(
	entry_ref	*directory,
	const char	*name)
{
	SetTitle(name);

	if (mEntry != NULL) {
		delete (mEntry);
		mEntry = NULL;
	}

	BDirectory dir(directory);
	mEntry = new BEntry(&dir, name);
	dir.GetEntry(mDirEntry);	

	Save();
	
	if (mWaitForSave)
		PostMessage(B_CLOSE_REQUESTED);
}


bool
CStyledEditWindow::QuitRequested()
{
	bool result = TRUE;
	mWaitForSave = FALSE;

	if (mDirty) {
		const char 	*text1 = kSave1;
		const char 	*text2 = kSave2;
		long		len = strlen(text1) + B_FILE_NAME_LENGTH + strlen(text2); 
		char		*title = (char *)malloc(len + 1);
		sprintf(title, "%s%s%s", text1, Title(), text2);
		
		BAlert *alert = new BAlert(B_EMPTY_STRING, title,
			kCancel, kDontSave, kSave,
			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		switch (alert->Go()) {
			case 0:
				// Cancel
				result = FALSE;
				break;
				
			case 1:
				// Don't Save
				break;
				
			default:
				if (mEntry == NULL) {
					result = FALSE;
					mWaitForSave = TRUE;
				}
				Save();
				break;
		}
		
		free(title);
	}
	 
	return (result);
}


void
CStyledEditWindow::MenusBeginning()
{
	BMenuItem *menuItem = NULL;

	menuItem = mFileMenu->FindItem(kSave);
	menuItem->SetEnabled(mDirty);

	menuItem = mFileMenu->FindItem(kSaveAs);
	menuItem->SetEnabled((mDirty) || (mEntry != NULL)); 

	menuItem = mFileMenu->FindItem(kRevert);
	menuItem->SetEnabled((mDirty) && (mEntry != NULL));

	bool		isRedo = false;
	undo_state	undoState = mTextView->UndoState(&isRedo);

	menuItem = mEditMenu->ItemAt(0);
	menuItem->SetLabel((isRedo) ? kRedoStrings[undoState] : kUndoStrings[undoState]);
	menuItem->SetEnabled(undoState != B_UNDO_UNAVAILABLE);

	long selStart = 0;
	long selEnd = 0;
	mTextView->GetSelection(&selStart, &selEnd);

	menuItem = mEditMenu->FindItem(kCut);
	menuItem->SetEnabled(selStart != selEnd);

	menuItem = mEditMenu->FindItem(kCopy);
	menuItem->SetEnabled(selStart != selEnd);

	menuItem = mEditMenu->FindItem(kPaste);
	menuItem->SetEnabled(mTextView->AcceptsPaste(be_clipboard));

	menuItem = mEditMenu->FindItem(kClear);
	menuItem->SetEnabled(selStart != selEnd);

	uint32		mode = 0;
	BFont		aFont;
	bool		colorEq = FALSE;
	rgb_color	aColor;
	mTextView->GetFontAndColor(&aFont, &mode, &aColor, &colorEq);

	// don't use FindMarked(), something in Size menu could be marked
	// menuItem = mFontMenu->FindMarked();
	for (int32 i = 3; (menuItem = mFontMenu->ItemAt(i)) != NULL; i++) {
		if (menuItem->IsMarked())
			break;
	}
	if (menuItem != NULL) {
		menuItem->SetMarked(FALSE);
		menuItem = menuItem->Submenu()->FindMarked();
		if (menuItem != NULL) {
			menuItem->SetMarked(FALSE);
		}
	}

	if (mode & B_FONT_FAMILY_AND_STYLE) {
		font_family	family;
		font_style	style;
		aFont.GetFamilyAndStyle(&family, &style);

		// don't use FindItem(), could have a font named 'Blue' for example...
		// menuItem = mFontMenu->FindItem(family);	
		for (int32 i = 3; (menuItem = mFontMenu->ItemAt(i)) != NULL; i++) {
			if (strcmp(menuItem->Label(), family) == 0)
				break;
		}
		
		if (menuItem != NULL) {
			menuItem->SetMarked(TRUE);
			menuItem = menuItem->Submenu()->FindItem(style);
			if (menuItem != NULL)
				menuItem->SetMarked(TRUE);
		}
	}

	menuItem = mSizeMenu->FindMarked();
	if (menuItem != NULL)
		menuItem->SetMarked(FALSE);
	if (mode & B_FONT_SIZE) {
		char numStr[3];
		sprintf(numStr, "%ld", (long)aFont.Size());
		menuItem = mSizeMenu->FindItem(numStr);
		if (menuItem != NULL)
			menuItem->SetMarked(TRUE);
	}

	menuItem = mColorMenu->FindMarked();
	if (menuItem != NULL)
		menuItem->SetMarked(FALSE);
	if (colorEq) {
		menuItem = NULL;
		CColorMenuItem *colorItem = NULL;
		for ( long i = 0; 
			  (colorItem = (CColorMenuItem *)mColorMenu->ItemAt(i)) != NULL;
			  i++) {
			if (colorItem->IsColorEqual(aColor)) {
				menuItem = colorItem;
				break;
			}
		}
		if (menuItem != NULL)
			menuItem->SetMarked(TRUE);
	}
}


void
CStyledEditWindow::SetDirty(
	bool	inDirty)
{
	mDirty = inDirty;
}


long
CStyledEditWindow::WindowCount()
{
	return (sWindowList.CountItems());
}


const BList*
CStyledEditWindow::WindowList()
{
	return (&sWindowList);
}

CStyledEditWindow*
CStyledEditWindow::FindWindow(
	entry_ref	*inRef)
{
	CStyledEditWindow *window = NULL;

	for ( long i = 0; 
		  (window = (CStyledEditWindow *)sWindowList.ItemAt(i)) != NULL; 
		  i++) {
		if (window->mEntry != NULL) {
			entry_ref theRef;
			window->mEntry->GetRef(&theRef);
			if (theRef == *inRef) {
				window->SetTitle(inRef->name);
				return (window);
			}
		}
	}
	
	return (NULL);
}


void
CStyledEditWindow::InitWindow(
	entry_ref	*parent)
{
	sWindowList.AddItem(this);
	be_app->PostMessage(msg_WindowAdded);
	
	Lock();

	float minH = 0.0;
	float maxH = 0.0;
	float minV = 0.0;
	float maxV = 0.0;
	GetSizeLimits(&minH, &maxH, &minV, &maxV);
	SetSizeLimits(100.0, maxH, 100.0, maxV);

	mEntry = NULL;
	mDirEntry = new BEntry(parent);
	mOpenPanel = NULL;
	mSavePanel = NULL;
	mDirty = FALSE;
	mWaitForSave = FALSE;
	mPrintSettings = NULL;
	mEncoding = kUTF8Conversion;
	m_is_source = false;
	
	BRect mbarRect = Bounds();
	mbarRect.bottom = mbarRect.top + 15.0;
	BMenuBar *mbar = new BMenuBar(mbarRect, "mbar");

	mFileMenu = new BMenu(kFile);	
	mbar->AddItem(mFileMenu);
	
	mEditMenu = new BMenu(kEdit);
	mbar->AddItem(mEditMenu);	

	mFontMenu = new BMenu(kFont);
	//mFontMenu->SetRadioMode(TRUE);
	mbar->AddItem(mFontMenu);

	mDocumentMenu = new BMenu(kDocument);
	mbar->AddItem(mDocumentMenu);

//	BMenu *infoMenu = new BMenu(kInfo);
//	mbar->AddItem(infoMenu);

	AddChild(mbar);

	BRect textFrame = Bounds();
	textFrame.top = mbar->Bounds().bottom + 1.0;
	textFrame.right -= B_V_SCROLL_BAR_WIDTH;
	textFrame.bottom -= B_H_SCROLL_BAR_HEIGHT;

	BRect textRect = textFrame;
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(3.0, 3.0);
	
	mTextView = new CTextView(textFrame, "text", textRect);
	BScrollView *scroller = new BScrollView("scroller", mTextView,
											B_FOLLOW_ALL_SIDES, 0,
											TRUE, TRUE, B_PLAIN_BORDER);
	AddChild(scroller);
	mTextView->MakeFocus();

	BMenuItem *fileItem = NULL;
	fileItem = new BMenuItem(kNew, new BMessage(msg_NewWindow), 'N');
	//fileItem->SetTarget(be_app);
	mFileMenu->AddItem(fileItem);
	fileItem = new BMenuItem(BRecentFilesList::NewFileListMenu(kOpen,
		NULL, NULL, be_app, 10, false, "text/plain", "application/x-vnd.Be-STEE"),
		new BMessage(msg_OpenWindow));
	fileItem->SetShortcut('O', B_COMMAND_KEY);
//	fileItem->SetTarget(be_app);
	mFileMenu->AddItem(fileItem);

#if xDEBUG // debug only, few recent menu permutations
	mFileMenu->AddItem(new BMenuItem(BRecentFilesList::NewFileListMenu("StyledEdit only",
		NULL, NULL, be_app, 20, false, NULL, "application/x-vnd.Be-STEE")));
	mFileMenu->AddItem(new BMenuItem(BRecentFilesList::NewFileListMenu("StyledEdit and any text",
		NULL, NULL, be_app, 20, false, "text", "application/x-vnd.Be-STEE")));
	const char *types[2] = {"text/html", "text/x-source-code"};
	mFileMenu->AddItem(new BMenuItem(BRecentFilesList::NewFileListMenu("html & code",
		NULL, NULL, be_app, 20, false, types, 2, "xxx/xxx")));
	mFileMenu->AddItem(new BMenuItem(BRecentFilesList::NewFileListMenu("StyledEdit, html & code",
		NULL, NULL, be_app, 20, false, types, 2, "application/x-vnd.Be-STEE")));
	mFileMenu->AddItem(new BMenuItem(BRecentFilesList::NewFileListMenu("any app, any type",
		NULL, NULL, be_app, 20, false, NULL, NULL)));
	mFileMenu->AddItem(new BMenuItem(BRecentAppsList::NewAppListMenu("Applications",
		NULL, NULL, 20)));
#endif

	mFileMenu->AddSeparatorItem();
	mFileMenu->AddItem(new BMenuItem(kSave, new BMessage(msg_Save), 'S'));
	mFileMenu->AddItem(new BMenuItem(kSaveAs, new BMessage(msg_SaveAs)));
	mFileMenu->AddItem(new BMenuItem(kRevert, new BMessage(msg_Revert)));
	mFileMenu->AddItem(new BMenuItem(kClose, new BMessage(B_CLOSE_REQUESTED), 'W'));
	mFileMenu->AddSeparatorItem();
	mFileMenu->AddItem(new BMenuItem(kPSetup, new BMessage(msg_PSetup)));
	mFileMenu->AddItem(new BMenuItem(kPrint, new BMessage(msg_Print), 'P'));
	mFileMenu->AddSeparatorItem();	
	fileItem = new BMenuItem(kQuit, new BMessage(B_QUIT_REQUESTED), 'Q');
	fileItem->SetTarget(be_app);
	mFileMenu->AddItem(fileItem);
	
	BMenuItem *editItem = NULL;
	editItem = new BMenuItem(kUndoStrings[B_UNDO_UNAVAILABLE], new BMessage(B_UNDO), 'Z');
	editItem->SetTarget(mTextView);
	mEditMenu->AddItem(editItem);
	mEditMenu->AddSeparatorItem();
	editItem = new BMenuItem(kCut, new BMessage(B_CUT), 'X');
	editItem->SetTarget(mTextView);
	mEditMenu->AddItem(editItem);
	editItem = new BMenuItem(kCopy, new BMessage(B_COPY), 'C');
	editItem->SetTarget(mTextView);
	mEditMenu->AddItem(editItem);
	editItem = new BMenuItem(kPaste, new BMessage(B_PASTE), 'V');
	editItem->SetTarget(mTextView);
	mEditMenu->AddItem(editItem);
	mEditMenu->AddItem(new BMenuItem(kClear, new BMessage(msg_Clear)));
	mEditMenu->AddSeparatorItem();
	mEditMenu->AddItem(new BMenuItem(kSelectAll, new BMessage(msg_SelectAll), 'A'));	
	mEditMenu->AddSeparatorItem();
	mEditMenu->AddItem(new BMenuItem(kFind, new BMessage(msg_Find), 'F'));
	mEditMenu->AddItem(new BMenuItem(kFindAgain, new BMessage(msg_FindAgain), 'G'));
	mEditMenu->AddItem(new BMenuItem(kFindSel, new BMessage(msg_FindSelection), 'H'));
	mEditMenu->AddItem(new BMenuItem(kReplace, new BMessage(msg_Replace), 'R'));
	mEditMenu->AddItem(new BMenuItem(kRSame, new BMessage(msg_ReplaceSame), 'T'));
	
	mSizeMenu = new BMenu(kSize);
	mSizeMenu->AddItem(new BMenuItem("9", new BMessage(msg_SizeSelected)));
	mSizeMenu->AddItem(new BMenuItem("10", new BMessage(msg_SizeSelected)));
	mSizeMenu->AddItem(new BMenuItem("12", new BMessage(msg_SizeSelected)));
	mSizeMenu->AddItem(new BMenuItem("14", new BMessage(msg_SizeSelected)));
	mSizeMenu->AddItem(new BMenuItem("18", new BMessage(msg_SizeSelected)));
	mSizeMenu->AddItem(new BMenuItem("24", new BMessage(msg_SizeSelected)));
	mSizeMenu->AddItem(new BMenuItem("36", new BMessage(msg_SizeSelected)));
	mSizeMenu->AddItem(new BMenuItem("48", new BMessage(msg_SizeSelected)));
	mSizeMenu->AddItem(new BMenuItem("72", new BMessage(msg_SizeSelected)));
	mFontMenu->AddItem(new BMenuItem(mSizeMenu));

	mColorMenu = new BMenu(kColor);
	mColorMenu->AddItem(new CColorMenuItem(kBlackT, kBlack, new BMessage(msg_ColorSelected)));
	mColorMenu->AddItem(new CColorMenuItem(kRedT, kRed, new BMessage(msg_ColorSelected)));
	mColorMenu->AddItem(new CColorMenuItem(kGreenT, kGreen, new BMessage(msg_ColorSelected)));
	mColorMenu->AddItem(new CColorMenuItem(kBlueT, kBlue, new BMessage(msg_ColorSelected)));
	mColorMenu->AddItem(new CColorMenuItem(kCyanT, kCyan, new BMessage(msg_ColorSelected)));
	mColorMenu->AddItem(new CColorMenuItem(kMagentaT, kMagenta, new BMessage(msg_ColorSelected)));
	mColorMenu->AddItem(new CColorMenuItem(kYellowT, kYellow, new BMessage(msg_ColorSelected)));	
	mFontMenu->AddItem(new BMenuItem(mColorMenu));

	mFontMenu->AddSeparatorItem();

	int32 numFamilies = count_font_families();
	for (int32 i = 0; i < numFamilies; i++) {
		font_family family;
		get_font_family(i, &family);
		
		BMenu	*styleMenu = new BMenu(family);
		int32	numStyles = count_font_styles(family);
		for (int32 j = 0; j < numStyles; j++) {
			font_style style;
			get_font_style(family, j, &style);
			styleMenu->AddItem(new BMenuItem(style, 
											 new BMessage(msg_StyleSelected)));
		}

		mFontMenu->AddItem(new BMenuItem(styleMenu, 
										 new BMessage(msg_FamilySelected)));
	}
		
	BMenu *alignMenu = new BMenu(kAlign);
	alignMenu->SetRadioMode(TRUE);
	
	BMenuItem *documentItem = NULL;
	documentItem = new BMenuItem(kLeft, new BMessage(msg_AlignLeft));
	documentItem->SetMarked(TRUE);
	alignMenu->AddItem(documentItem);
	alignMenu->AddItem(new BMenuItem(kCenter, new BMessage(msg_AlignCenter)));
	alignMenu->AddItem(new BMenuItem(kRight, new BMessage(msg_AlignRight)));
	mDocumentMenu->AddItem(new BMenuItem(alignMenu));

	documentItem = new BMenuItem(kWrap, new BMessage(msg_SetWrap));
	documentItem->SetMarked(mTextView->DoesWordWrap());
	mDocumentMenu->AddItem(documentItem);

/*
	BMenu *backColorMenu = new BMenu("Background");
	backColorMenu->SetRadioMode(TRUE);
	BMenuItem *whiteBack = new CColorMenuItem("White", kWhite, new BMessage(msg_BackColorSelected), FALSE);
	whiteBack->SetMarked(TRUE);
	backColorMenu->AddItem(whiteBack);
	backColorMenu->AddItem(new CColorMenuItem("Gray", kGray, new BMessage(msg_BackColorSelected), FALSE));
	backColorMenu->AddItem(new CColorMenuItem("Yellow", kBackYellow, new BMessage(msg_BackColorSelected), FALSE));
	backColorMenu->AddItem(new CColorMenuItem("Green", kBackGreen, new BMessage(msg_BackColorSelected), FALSE));
	backColorMenu->AddItem(new CColorMenuItem("Blue", kBackBlue, new BMessage(msg_BackColorSelected), FALSE));
	backColorMenu->AddItem(new CColorMenuItem("Pink", kPink, new BMessage(msg_BackColorSelected), FALSE));
	backColorMenu->AddItem(new CColorMenuItem("Purple", kPurple, new BMessage(msg_BackColorSelected), FALSE));	
	documentMenu->AddItem(new BMenuItem(backColorMenu));	
*/
//	BMenuItem *infoItem = new BMenuItem(kAbout, new BMessage(B_ABOUT_REQUESTED));
//	infoItem->SetTarget(be_app);
//	infoMenu->AddItem(infoItem);
//	infoMenu->AddItem(new BMenuItem(kHelp, NULL));

	Unlock();
}


void
CStyledEditWindow::Find()
{
	CStyledEditApp *theApp = (CStyledEditApp *)be_app;

	if (!theApp->GetFindString(FALSE, this))
		return;
	
	UpdateIfNeeded();

	mTextView->Search(theApp->SearchString(), theApp->SearchForward(), 
					  theApp->SearchWrap(), theApp->SearchSensitive());
}


void
CStyledEditWindow::FindAgain()
{
	CStyledEditApp *theApp = (CStyledEditApp *)be_app;

	mTextView->Search(theApp->SearchString(), theApp->SearchForward(), 
					  theApp->SearchWrap(), theApp->SearchSensitive());
}


void
CStyledEditWindow::FindSelection()
{
	int32 selStart = 0;
	int32 selEnd = 0;
	mTextView->GetSelection(&selStart, &selEnd);

	if (selStart == selEnd)
		return;

	CStyledEditApp *theApp = (CStyledEditApp *)be_app;

	int32 length = selEnd - selStart;
	length = (length > MAX_BUFFER_LEN) ? MAX_BUFFER_LEN : length;
	mTextView->GetText(selStart, length, theApp->SearchString());
	mTextView->Search(theApp->SearchString(), theApp->SearchForward(), 
					  theApp->SearchWrap(), theApp->SearchSensitive());

}


void
CStyledEditWindow::Replace()
{
	CStyledEditApp *theApp = (CStyledEditApp *)be_app;

	if (!theApp->GetFindString(TRUE, this))
		return;
	
	UpdateIfNeeded();

	if (theApp->ReplaceAll())
		mTextView->Select(0, 0);

	mTextView->Replace(!theApp->ReplaceAll(), theApp->SearchString(), theApp->ReplaceString(), 
					   theApp->SearchForward(), theApp->SearchWrap(), 
					   theApp->SearchSensitive());	
}


void
CStyledEditWindow::ReplaceSame()
{
	CStyledEditApp *theApp = (CStyledEditApp *)be_app;

	mTextView->Select(0, 0);
	mTextView->Replace(FALSE, theApp->SearchString(), theApp->ReplaceString(), 
					   theApp->SearchForward(), theApp->SearchWrap(), 
					   theApp->SearchSensitive());
}


void
CStyledEditWindow::FontSelected(
	const font_family	family,
	const font_style	style)
{
	BView *theView = CurrentFocus();
	if ((theView != NULL) && (is_kind_of(theView, BTextView))) {
		BFont newStyle;
		newStyle.SetFamilyAndStyle(family, style);
		((BTextView *)theView)->SetFontAndColor(&newStyle, 
												B_FONT_FAMILY_AND_STYLE);
		SetDirty(true);
	}
}


void
CStyledEditWindow::SizeSelected(
	float	size)
{
	BView *theView = CurrentFocus();
	if ((theView != NULL) && (is_kind_of(theView, BTextView))) {
		BFont newStyle;
		newStyle.SetSize(size);
		((BTextView *)theView)->SetFontAndColor(&newStyle, B_FONT_SIZE);
		SetDirty(true);
	}
}


void
CStyledEditWindow::ColorSelected(
	rgb_color	color,
	bool		penColor)
{
	BView *theView = CurrentFocus();
	if ((theView != NULL) && (is_kind_of(theView, BTextView))) {
		if (penColor) {
			((BTextView *)theView)->SetFontAndColor(NULL, 0, &color);
			SetDirty(true);
		}
		else {
			theView->SetViewColor(color);
			theView->Invalidate();
		}
	}
}


void
CStyledEditWindow::SetWrapping(
	bool	wrap)
{
	BMenuItem *item = mDocumentMenu->FindItem(kWrap);
	item->SetMarked(wrap);

	mTextView->SetWordWrap(wrap);
	BRect textRect = mTextView->TextRect();
	if (!wrap)
		textRect.right = textRect.left + 1500.0;
	else {
		textRect = mTextView->Frame();
		textRect.OffsetTo(B_ORIGIN);
		textRect.InsetBy(3.0, 3.0);
	}
	mTextView->SetTextRect(textRect);
}


void
CStyledEditWindow::Save()
{
	if (mEntry == NULL)
		SaveAs();
	else
		WriteData();
}


void
CStyledEditWindow::SaveAs()
{
	BMessenger self(this);

	if (!mSavePanel) {
		entry_ref dirEntry;
		mDirEntry->GetRef(&dirEntry);
		mSavePanel = new BFilePanel(B_SAVE_PANEL, &self, &dirEntry);
		mSavePanel->SetSaveText(Title());
		if (mEntry != NULL)
			mSavePanel->Window()->SetTitle("Save As");
		
		AddEncodingMenuToFilePanel(mSavePanel, mEncoding);
	}

	mSavePanel->Show();
}


void
CStyledEditWindow::Revert()
{ 
	const char 	*text1 = kRevert1;
	const char 	*text2 = kRevert2;
	long		len = strlen(text1) + B_FILE_NAME_LENGTH + strlen(text2); 
	char		*title = (char *)malloc(len + 1);
	sprintf(title, "%s%s%s", text1, Title(), text2);
	
	BAlert *alert = new BAlert(B_EMPTY_STRING, title, kCancel, kOK, NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert->Go() != 0)
		ReadData();
}


void
CStyledEditWindow::WriteData()
{
	if (mEntry == NULL)
		return;

	BFile file(mEntry, O_RDWR);

	if (file.InitCheck() == B_NO_ERROR) {
		mode_t perms = 0;
		file.GetPermissions(&perms);
	
		if ((perms & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0) {
			(new BAlert(B_EMPTY_STRING, "File is read-only.", "OK"))->Go();
			return;
		}
	}
	else {
		BDirectory dir;
		mEntry->GetParent(&dir);

		entry_ref ref;
		mEntry->GetRef(&ref);

		dir.CreateFile(ref.name, &file);

		if (file.InitCheck() != B_NO_ERROR) {
			delete (mEntry);
			mEntry = NULL;
			(new BAlert(B_EMPTY_STRING,
						"An error occured while saving the file.", 
						"OK"))->Go(); 
			return;
		}
	}

	// text
	int32		saveTextLen = mTextView->TextLength();
	const char	*saveText = mTextView->Text();
	int32		theTextLen = 0;
	char		*theText = NULL;

	if (mEncoding != kUTF8Conversion) {
		const size_t kDstSize = 1024 * 32;

		int32	offset = 0;
		int32	state = 0;
		char	*dst = (char *)malloc(kDstSize);

		while (offset < saveTextLen) {
			const char	*src = saveText + offset;
			int32		srcLen = saveTextLen - offset;	
			int32		dstLen = kDstSize;
	
			convert_from_utf8(mEncoding, src, &srcLen, dst, &dstLen, &state);

			theText = (char *)realloc(theText, theTextLen + dstLen);
			memcpy(theText + theTextLen, dst, dstLen);
			theTextLen += dstLen;			
			offset += srcLen;
		}		

		free(dst);

		saveTextLen = theTextLen;
		saveText = theText;
	}

	file.Seek(0, SEEK_SET);
	file.SetSize(0);
	file.Write(saveText, saveTextLen);

	if (theText != NULL)
		free(theText);

	// wrapping
	file.RemoveAttr("wrap");
	int32 wrapState = (mTextView->DoesWordWrap()) ? 1 : 0;
	file.WriteAttr("wrap", B_INT32_TYPE, 0, &wrapState, sizeof(int32));

	// alignment
	file.RemoveAttr("alignment");
	int32 alignState = mTextView->Alignment();
	file.WriteAttr("alignment", B_INT32_TYPE, 0, &alignState, sizeof(int32));

	// styles
	file.RemoveAttr("styles");

	text_run_array	*textRuns = mTextView->RunArray(0, mTextView->TextLength());
	int32			dataSize = 0;
	void			*data = BTextView::FlattenRunArray(textRuns, &dataSize);

	file.WriteAttr("styles", B_RAW_TYPE, 0, data, dataSize);

	BTextView::FreeRunArray(textRuns);
	free(data);

	// encoding
	file.RemoveAttr("be:encoding");
	file.WriteAttr("be:encoding", B_INT32_TYPE, 0, &mEncoding, sizeof(uint32));

#if ALLOW_WRITE_TYPE_ATTR
	// this should not be added in an iad build
	// make that ol' icon show up
	BNodeInfo	nodeInfo(&file);
	char		mimeType[B_MIME_TYPE_LENGTH + 1] = "";
	if (nodeInfo.GetType(mimeType) != B_NO_ERROR)	
		nodeInfo.SetType(m_is_source ? "text/x-source-code" : "text/plain");
#endif

	SetDirty(FALSE);
}


void
CStyledEditWindow::ReadData()
{
	if (mEntry == NULL)
		return;

	BFile file(mEntry, O_RDONLY);
	if (file.InitCheck() != B_NO_ERROR) {
		delete (mEntry);
		mEntry = NULL;
		return;
	}

	mode_t perms = 0;
	file.GetPermissions(&perms);
	if ((perms & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
		SetReadOnly();

	// is this source code?
	m_is_source = false;
	BNodeInfo info(&file);
	char type[256] = "";
	if (!info.GetType(type) && !strcasecmp(type, "text/x-source-code")) {
		m_is_source = true;
	}

	file.Seek(0, SEEK_SET);
	off_t len = 0;
	file.GetSize(&len);

	if (len > 0) {
		attr_info attrInfo;

		// wrapping
		if (file.GetAttrInfo("wrap", &attrInfo) == B_NO_ERROR) {
			if (attrInfo.size == sizeof(int32)) {
				int32 wrapState = 0;
				file.ReadAttr("wrap", B_INT32_TYPE, 0, 
							  &wrapState, attrInfo.size);
				SetWrapping((wrapState == 0) ? false : true);
			}
		}
		else if (m_is_source) {
			SetWrapping(false);
		}

		// alignment
		status_t err;
		if (((err = file.GetAttrInfo("alignment", &attrInfo)) == B_NO_ERROR) || m_is_source) {
			int32 alignState = 0;
			if (!err && (attrInfo.size == sizeof(int32))) {
				file.ReadAttr("alignment", B_INT32_TYPE, 0, 
							  &alignState, attrInfo.size);
			}
			else if (m_is_source) {
				alignState = B_ALIGN_LEFT;
			}
			mTextView->SetAlignment((alignment)alignState);

			const char *itemName = NULL;
			switch (alignState) {
				case B_ALIGN_LEFT:
					itemName = kLeft;
					break;

				case B_ALIGN_RIGHT:
					itemName = kRight;
					break;

				case B_ALIGN_CENTER:
					itemName = kCenter;
					break;
			}
			BMenuItem *item = mDocumentMenu->FindItem(itemName);
			item->SetMarked(true);
		}

		// styles
		text_run_array	*textRuns = NULL;
		if ((file.GetAttrInfo("styles", &attrInfo) == B_NO_ERROR)) {
			if (attrInfo.size > 0) {
				void *data = malloc(attrInfo.size);
				file.ReadAttr("styles", B_RAW_TYPE, 0, data, attrInfo.size);
				textRuns = BTextView::UnflattenRunArray(data);
				free(data);
			}
		}
		else if (m_is_source) {
			textRuns = BTextView::AllocRunArray(1);
			textRuns->count = 1;
			textRuns->runs[0].offset = 0;
			textRuns->runs[0].font = *be_fixed_font;
			rgb_color black = { 0, 0, 0, 255 };
			textRuns->runs[0].color = black;
		}

		// text
		if (mEncoding == kUTF8Conversion)
			mTextView->SetText(&file, 0, len);
		else {
			mTextView->Delete(0, LONG_MAX);

			const size_t kSrcSize = 1024 * 32;
			const size_t kDstSize = kSrcSize * 4;

			int32	state = 0;
			char	*dst = (char *)malloc(kDstSize);
			char	*src = (char *)malloc(kSrcSize);
			int32	srcLen = 0;
			int32	offset = 0;

			while ((srcLen = file.Read(src + offset, kSrcSize - offset)) > 0) {
				srcLen += offset;

				int32 dstLen = kDstSize;
				int32 origSrcLen = srcLen;

				convert_to_utf8(mEncoding, src, &srcLen, dst, &dstLen, &state);

				if (srcLen < origSrcLen) {
					offset = origSrcLen - srcLen;
					memmove(src, src + srcLen, offset);
				}
				else
					offset = 0;
	
				mTextView->Insert(dst, dstLen);
			}		

			free(src);
			free(dst);

			mTextView->Select(0, 0);
		}			
		if(textRuns)	
			mTextView->SetRunArray(0, mTextView->TextLength(), textRuns);
		// Set the text run arrays
		SetDirty(FALSE);

		BTextView::FreeRunArray(textRuns);
	}
}


status_t
CStyledEditWindow::PageSetup()
{
	status_t	result = B_ERROR;
	BPrintJob	printJob(Title());
	
	if (mPrintSettings != NULL)
		printJob.SetSettings(new BMessage(*mPrintSettings));

	result = printJob.ConfigPage(); 
	
	if (result == B_NO_ERROR) {
		delete (mPrintSettings);
		mPrintSettings = printJob.Settings();
	}

	return (result);
}


void
CStyledEditWindow::Print()
{
	if (mPrintSettings == NULL) {
		if (PageSetup() != B_NO_ERROR)
			return;
	}

	BPrintJob printJob(Title());
	printJob.SetSettings(new BMessage(*mPrintSettings));

	if (printJob.ConfigJob() == B_NO_ERROR) {
		int32	curPage = 1;
		int32	lastLine = 0;
		int32	maxLine = mTextView->CountLines() - 1;
		BRect	pageRect = printJob.PrintableRect();
		BRect	curPageRect = pageRect;

		printJob.BeginJob();

		do {
			int32 lineOffset = mTextView->OffsetAt(lastLine);
			curPageRect.OffsetTo(0, mTextView->PointAt(lineOffset).y);

			int32 fromLine = lastLine;
			lastLine = mTextView->LineAt(BPoint(0.0, curPageRect.bottom));

			float curPageHeight = mTextView->TextHeight(fromLine, lastLine);
			if (curPageHeight > pageRect.Height())
				curPageHeight = mTextView->TextHeight(fromLine, --lastLine);
			curPageRect.bottom = curPageRect.top + curPageHeight - 1.0;
			
			if ((curPage >= printJob.FirstPage()) && (curPage <= printJob.LastPage())) {
				printJob.DrawView(mTextView, curPageRect, BPoint(0.0, 0.0));
				printJob.SpoolPage();
			}

			curPageRect = pageRect;
			lastLine++;
			curPage++;
		} while ((printJob.CanContinue()) && (lastLine < maxLine));

		printJob.CommitJob();
	}
}


void
CStyledEditWindow::SetReadOnly()
{
	mEditMenu->FindItem(kReplace)->SetEnabled(false);
	mEditMenu->FindItem(kRSame)->SetEnabled(false);
	mSizeMenu->Supermenu()->SetEnabled(false);
	mDocumentMenu->SetEnabled(false);
	mTextView->MakeEditable(false);
}


void
CStyledEditWindow::AddEncodingMenuToFilePanel(
	BFilePanel	*panel,
	uint32		encoding)
{
	BMenuBar *panelMBAR = dynamic_cast<BMenuBar*>(panel->Window()->FindView("MenuBar"));

	if (panelMBAR != NULL) {
		BMenu *encodingsMenu = new BMenu("Encoding");
		encodingsMenu->SetRadioMode(true);

		for (int32 i = 0; i < kNumEncodings; i++) {
			BMenuItem *item = new BMenuItem(kEncodings[i].name, NULL);
			if (kEncodings[i].flavor == encoding)
				item->SetMarked(true);
			encodingsMenu->AddItem(item);
		}

		panelMBAR->AddItem(encodingsMenu);
	}	
}


uint32
CStyledEditWindow::EncodingSettingOfFilePanel(
	BFilePanel	*panel)
{
	BMenuBar *panelMBAR = dynamic_cast<BMenuBar*>(panel->Window()->FindView("MenuBar"));

	if (panelMBAR != NULL) {
		BMenuItem *encodingsMenuItem = panelMBAR->FindItem("Encoding");

		if (encodingsMenuItem != NULL) {
			BMenuItem *item = encodingsMenuItem->Submenu()->FindMarked();

			if (item != NULL) {
				for (int32 i = 0; i < kNumEncodings; i++) {
					if (strcmp(item->Label(), kEncodings[i].name) == 0)
						return (kEncodings[i].flavor);
				}
			}
		}
	}

	return (kUTF8Conversion);
}


BRect
CStyledEditWindow::GetFrameRect()
{
	BScreen screen( B_MAIN_SCREEN_ID );
	BRect sframe = screen.Frame();
	BRect frame;
	
	frame.left = sframe.left + 7.0;
	frame.top = sframe.top + 26.0;
	frame.right = frame.left + 500.0;
	frame.bottom = frame.top + 400.0;
	
	BRect 	offsetFrame = frame;
	long	numWindows = sWindowList.CountItems();
	offsetFrame.OffsetBy(15.0 * numWindows, 15.0 * numWindows);
	if (screen.Frame().Contains(offsetFrame))
		frame = offsetFrame;
		
	return (frame);
}
