//
//	Browser interface
//
#include "JapaneseIMContent.h"
#include "BitmapData.h"
#include "ModeButton.h"

#include <Bitmap.h>
#include <Button.h>
#include <Debug.h>
#include <Input.h>
#include <JapaneseCommon.h>
#include <ListView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <TextView.h>
#include <UTF8.h>
#include <stdio.h>

#define SET_BOTTOMLINE_MESSENGER	'_SBL'	/* used by a browser plugin to give the input_server
											 * a messenger to something that handles the bottomline
											 * window functionality. */
#define ACTIVATE_BOTTOMLINE			'_ABL'	/* tells the browser plugin to activate the bottomline */
#define BOTTOMLINE_EVENTS			'_BLE'	/* used to send events from the bottomline to the input server */

inline int32 utf8_char_len(uchar c)
{ 
	return (((0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1);
}

//----------------------------------------

const rgb_color kBlack = {0,0,0,255};
const rgb_color kDarkBorder = {120,120,120,255};
const rgb_color kLightBorder = {230,230,230,255};
const rgb_color kInterior = {200,200,200,255};
const rgb_color kInteriorHilite = {128,128,128,255};
const float kButtonRadius = 4.0f;
const float kBorderWidth = 10.0f;
const uint32 kTextInvokeMsg = 'tCIK';
//----------------------------------------

class MyTextView: public BTextView
{
public:
					MyTextView(BRect rect, const char *name, BRect textrect,
							   uint32 resizingmode, uint32 flags);
	virtual			~MyTextView();
	
	virtual void	MakeFocus(bool focus);

	BView *previousfocus;
};

//----------------------------------------

class MyTextControl: public BTextControl
{
public:
					MyTextControl(BRect rect, const char *name, BMessage *message,
							   uint32 resizingmode, uint32 flags);
	virtual			~MyTextControl();
	
	virtual void	MakeFocus(bool focus);

	BView*		fPrevFocus;
};

//----------------------------------------

class MyListView: public BListView
{
public:
					MyListView(BRect rect);
	virtual			~MyListView();

	virtual	void	MakeFocus(bool focus);

	void			Clear();
};

//----------------------------------------

class MyKey: public BButton
{
public:
					MyKey(BRect rect, const char *label, BMessage *message);
	virtual			~MyKey();

	virtual	void	AttachedToWindow();
	virtual	void	Draw(BRect);

private:
  	BRect size;
};

//----------------------------------------

class MyContainerView: public BView
{
public:
					MyContainerView();
	virtual			~MyContainerView();

	virtual	void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual	void	MessageReceived(BMessage *mes);
			bool	HandleInputMethodEvent(BMessage *event, BList *events);
			void	GenerateKeyDowns(const char *string, BList *events);

private:
	BMessenger		fJapanMessenger;
	ModeButton*		fModeButton;
	MyTextControl*	fText;
};

//----------------------------------------

status_t JIMMessenger(BMessenger *msngr)
{
	// get a messenger to the japanese input method
	BMessage    methodAddress;
	ssize_t     size = 0;
	char        *buf = NULL;
	int32       code = 0;
	status_t    err = B_ERROR;

	port_id dropBox = find_port(J_DROP_BOX_NAME);
	if (dropBox < 0)
		return B_ERROR;

	size = port_buffer_size(dropBox);
	buf = (char *)malloc(size);
	code = 0;

	read_port(dropBox, &code, buf, size);
	err = methodAddress.Unflatten(buf);

	free(buf);

	if (err != B_NO_ERROR)
		return err;

	BMessenger sMethod;
	if (methodAddress.FindMessenger(J_MESSENGER, &sMethod) != B_NO_ERROR)
		return B_ERROR;

	sMethod.SendMessage(J_GRABBED_DROP_BOX); // this causes the port to be filled with another messenger

	*msngr=sMethod;
	return B_OK;
}

//----------------------------------------

MyTextView::MyTextView(BRect rect, const char *name, BRect textrect,
					   uint32 resizingmode,uint32 flags)
	: BTextView(rect, name, textrect, resizingmode, flags)
{
	previousfocus = NULL;
}

MyTextView::~MyTextView()
{
}

void MyTextView::MakeFocus(bool focus)
{
	if (focus) {
		BView *curfocus = Window()->CurrentFocus();
		if (curfocus != this) {
			previousfocus = curfocus;
		}
	}
	else {
		previousfocus=NULL;
	}
	BTextView::MakeFocus(focus);
}

//----------------------------------------

MyTextControl::MyTextControl(BRect rect, const char *name, BMessage *message,
							 uint32 resizingmode, uint32 flags)
	: BTextControl(rect, name, B_EMPTY_STRING, B_EMPTY_STRING, message, resizingmode, flags)
{
	fPrevFocus = NULL;
}

MyTextControl::~MyTextControl()
{
}

void MyTextControl::MakeFocus(bool focus)
{
	if (focus) {
		BView *curfocus = Window()->CurrentFocus();
		if (curfocus != TextView()) {
			fPrevFocus = curfocus;
		}
	}
	else {
		fPrevFocus = NULL;
	}
	//BTextControl::MakeFocus(focus);
}

//----------------------------------------

MyListView::MyListView(BRect rect)
	: BListView(rect, "")
{
}

MyListView::~MyListView()
{
}

void MyListView::MakeFocus(bool focus)
{
	BListView::MakeFocus(false);
}

void MyListView::Clear()
{
	for (int i = CountItems() - 1; i >= 0; i--) {
		BListItem *item = RemoveItem(i);
		if (!item) {
			break;
		}
		delete item;
	}
}


MyKey::MyKey(BRect rect, const char *label, BMessage *message)
	: BButton(rect,"",label,message,B_FOLLOW_NONE,B_WILL_DRAW)
{
	size = rect;	
}

MyKey::~MyKey()
{
}

void MyKey::AttachedToWindow()
{
	BButton::AttachedToWindow();
	ResizeTo(size.Width(),size.Height());
}

void MyKey::Draw(BRect)
{
	rgb_color interiorColor;
	//
	//	fill the interior
	if (Value()) {
		interiorColor = kInteriorHilite;
	} else {
		interiorColor = kInterior;
	}
	SetLowColor(interiorColor);
	SetHighColor(interiorColor);

	BRect frame(Bounds());
	frame.InsetBy(2, 2);
	frame.right--;
	frame.bottom--;
	FillRoundRect(frame, kButtonRadius, kButtonRadius);

	//
	//	draw the border
	frame.InsetBy(-2, -2);
	
	frame.OffsetBy(1, 1);
	SetHighColor(kLightBorder);
	StrokeRoundRect(frame, kButtonRadius, kButtonRadius);
	frame.OffsetBy(-1, -1);
	SetHighColor(kDarkBorder);
	StrokeRoundRect(frame, kButtonRadius, kButtonRadius);

	//
	//	draw the text
	const char* label = Label();
	if (label && strlen(label) > 0) {
		SetHighColor(kBlack);
		MovePenTo(BPoint(10, 15));		
		DrawString(label);
	}
}


MyContainerView::MyContainerView()
	: BView(BRect(0, 0, 200, 200),"*japanese", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
		fModeButton(NULL), fText(NULL)
{
	rgb_color color= ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(color);
}

MyContainerView::~MyContainerView()
{
}

void MyContainerView::AttachedToWindow()
{
	// fetch messenger to Japanese input method
	JIMMessenger(&fJapanMessenger);

	BRect r = Bounds();
	// calculate space for mode button
	BRect buttonRect = r;
	buttonRect.InsetBy(kBorderWidth, kBorderWidth);
	buttonRect.right = buttonRect.left + kButtonBitsWidth;
	buttonRect.bottom = buttonRect.top + kButtonBitsHeight;
	
	// add textview
	BRect textRect = buttonRect;
	textRect.OffsetBy(buttonRect.Width() + kBorderWidth, 0.0f);
	textRect.right = r.right - kBorderWidth;
	fText = new MyTextControl(textRect, B_EMPTY_STRING, new BMessage(kTextInvokeMsg),
							  B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
	BFont font(be_plain_font);
	font.SetSize(14);
	//const rgb_color white = {255,255,255,255};
	//const rgb_color black = {0,0,0,255};
	//fText->SetViewColor(white);
	//fText->TextView()->SetFontAndColor(&font, B_FONT_ALL, &black);
	fText->SetDivider(0);
	fText->SetEnabled(false);
	fText->Hide();
	fText->SetTarget(this);
	fText->TextView()->SetFlags(fText->TextView()->Flags() & ~B_INPUT_METHOD_AWARE & ~B_NAVIGABLE);
	AddChild(fText);

	if (fJapanMessenger.IsValid()) {
		// get current mode
		BMessage question(J_GET_INPUT_MODE);
		BMessage answer;
		fJapanMessenger.SendMessage(&question,&answer);
		int32 mode = answer.FindInt32("inputmode");
	
		// add mode button
		fModeButton = new ModeButton(buttonRect, &fJapanMessenger, mode);
		fModeButton->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);
		AddChild(fModeButton);
		
		// tell the JIM about the mode button
		BMessage modeMsg(J_SET_MODE_MESSENGER);
		modeMsg.AddMessenger(J_MODE_MESSENGER_NAME, BMessenger(fModeButton));
		fJapanMessenger.SendMessage(&modeMsg);
	}
	
	// tell the input server that we can handle bottomline input
	BMessenger inputMessenger("application/x-vnd.Be-input_server");
	if (inputMessenger.IsValid()) {
		BMessenger selfMessenger(this);
		BMessage msg(SET_BOTTOMLINE_MESSENGER);
		msg.AddMessenger("bottomline", selfMessenger);
		inputMessenger.SendMessage(&msg, (BHandler*)NULL, 1000000LL);
	}
}

void MyContainerView::DetachedFromWindow()
{
	// send an invalid messenger to the JIM in order to clear out
	// the messenger to the mode button
	if (fJapanMessenger.IsValid()) {
		BMessage modeMsg(J_SET_MODE_MESSENGER);
		modeMsg.AddMessenger(J_MODE_MESSENGER_NAME, BMessenger());
		fJapanMessenger.SendMessage(&modeMsg);
	}

	// tell the input server that we can no longer handle bottomline input
	BMessenger inputMessenger("application/x-vnd.Be-input_server");
	if (inputMessenger.IsValid()) {
		// don't add a messenger to the message, so the input server will
		// unset the bottomline messenger
		BMessage msg(SET_BOTTOMLINE_MESSENGER);
		inputMessenger.SendMessage(&msg, (BHandler*)NULL, 1000000LL);
	}
}

void MyContainerView::MessageReceived(BMessage *mes)
{	
	switch(mes->what) {
	case ACTIVATE_BOTTOMLINE:
		fText->SetEnabled(true);
		if (fText->IsHidden()) {
			fText->Show();
		}
		break;
	case B_CLOSE_REQUESTED:
		if (!fText->IsHidden()) {
			fText->Hide();
		}
		fText->SetEnabled(false);
		break;
	case B_INPUT_METHOD_EVENT:
		{
			// this code is taken almost verbatim from BottomlineWindow.cpp
			BMessage reply(BOTTOMLINE_EVENTS);
			BList events;
			bool r = HandleInputMethodEvent(mes, &events);
			if (!r) {
				reply.AddBool("cancel_im", false);
			}
			BMessage *m = NULL;
			while ((m = (BMessage *)events.RemoveItem((int32)0)) != NULL) {
				reply.AddMessage("generated_event", m);
			}
			mes->SendReply(&reply, (BHandler*)NULL, 1000000LL);
		}
		break;
//	case 'left':
//		{
//			BMessage mes(B_KEY_DOWN);
//			mes.AddInt64("when",system_time());
//			mes.AddInt32("modifiers",0);
//			mes.AddInt32("key",0x61);
//			mes.AddInt8("byte",0x1c);
//			fJapanMessenger.SendMessage(&mes);
//		}
//		break;
//	case 'xlft':
//		{
//			BMessage mes(B_KEY_DOWN);
//			mes.AddInt64("when",system_time());
//			mes.AddInt32("modifiers",0x201);
//			mes.AddInt32("key",0x61);
//			mes.AddInt8("byte",0x1c);
//			fJapanMessenger.SendMessage(&mes);
//		}
//		break;
//	case 'rite':
//		{
//			BMessage mes(B_KEY_DOWN);
//			mes.AddInt64("when",system_time());
//			mes.AddInt32("modifiers",0);
//			mes.AddInt32("key",0x63);
//			mes.AddInt8("byte",0x1d);
//			fJapanMessenger.SendMessage(&mes);
//		}
//		break;
//	case 'xrit':
//		{
//			BMessage mes(B_KEY_DOWN);
//			mes.AddInt64("when",system_time());
//			mes.AddInt32("modifiers",0x201);
//			mes.AddInt32("key",0x63);
//			mes.AddInt8("byte",0x1d);
//			fJapanMessenger.SendMessage(&mes);
//		}
//		break;
	case 'entr':
		if (fText->fPrevFocus || Window()->CurrentFocus() != fText->TextView())
		{
			if (fText->fPrevFocus) {
				fText->fPrevFocus->MakeFocus(true);
			}
			BList list;
			GenerateKeyDowns(fText->Text(), &list);
			for (int i = 0; ; i++) {
				BMessage *key = (BMessage*)list.ItemAt(i);
				if (!key) {
					break;
				}
				key->what = J_FAKED_KEYPRESS;
				fJapanMessenger.SendMessage(key);
				delete key;
			}
		}
		break;
//	case 'klik':
//		mes->what = J_SET_KOUHO_LIST_SELECTION;
//		fJapanMessenger.SendMessage(mes);
//		break;
	case msg_HiraganaInput:
	case msg_ZenkakuKatakanaInput:
	case msg_ZenkakuEisuuInput:
	case msg_HankakuKatakanaInput:
	case msg_HankakuEisuuInput:
	case msg_DirectInput:
	case msg_DirectHiraInput:
	case msg_DirectKataInput:
		fJapanMessenger.SendMessage(mes);
		break;
//	case J_SET_KOUHO_LIST_SELECTION:
//		{
//			int32 num;
//			list->SetSelectionMessage(new BMessage);
//			if (B_OK == mes->FindInt32("selectionindex",&num)) {
//				list->Select(num);
//			}
//			list->ScrollToSelection();
//			list->SetSelectionMessage(new BMessage('klik'));
//		}
//		break;
//	case J_SET_KOUHO_LISTVIEW:
//		list->Clear();
//		for (int i = 0; ; i++) {
//			const char *kouhostring;
//			if(B_OK != mes->FindString("kouho",i,&kouhostring))
//				break;
//			list->AddItem(new BStringItem(kouhostring));
//		}
//		break;
	default:
		BView::MessageReceived(mes);
		break;
	}
}

// This code is copied almost verbatim from BottomlineWindow.cpp in the input_server
bool MyContainerView::HandleInputMethodEvent(
	BMessage	*event,
	BList		*events)
{
	bool	result = true;
	uint32	opcode = 0;
	event->FindInt32("be:opcode", (int32 *)&opcode);

	switch (opcode) {
		case B_INPUT_METHOD_CHANGED:
		{
			const char *string = NULL;
			event->FindString("be:string", &string);

			if ((string == NULL) || (strlen(string) < 1)) {
				result = false;
				break;
			}

			bool confirmed = false;
			event->FindBool("be:confirmed", &confirmed);

			if (confirmed)
				GenerateKeyDowns(string, events);	
			break;
		}

		case B_INPUT_METHOD_STOPPED:
			// return false so that the bottomline textview will get disabled
			result = false;
			break;

		default:
			break;
	}

	Window()->PostMessage(event, fText->TextView());
	return (result);
}

// This method is copied almost verbatim from BottomlineWindow.cpp in the input_server
void MyContainerView::GenerateKeyDowns(const char *string, BList *events)
{
	int32 textLen = strlen(string);
	if (textLen < 1) {
		return;
	}
	const int32	kZeros[4] = {0, 0, 0, 0};
	char		*text = strdup(string);
	bigtime_t	when = system_time();

	int32 offset = 0;
	int32 numEventsGenerated = 0;
	while (offset < textLen) {
		int32 charLen = utf8_char_len(text[offset]);

		BMessage *event = new BMessage(B_KEY_DOWN);
		event->AddInt64("when", when);
		event->AddInt32("key", 0);
		event->AddInt32("modifiers", 0);
		event->AddInt32("raw_char", B_RETURN);
		event->AddData("states", B_UINT8_TYPE, kZeros, sizeof(kZeros));	

		for (int32 i = 0; i < charLen; i++) {
			event->AddInt8("byte", text[offset + i]);
		}
		char saveChar = text[offset + charLen];
		text[offset + charLen] = '\0';
		event->AddString("bytes", text + offset);
		text[offset + charLen] = saveChar;

		events->AddItem(event, numEventsGenerated);

		offset += charLen;
		numEventsGenerated++;
	}

	free(text);
}


// ---------------------- JapaneseIMContentInstance ----------------------

BView *JapaneseIMContentInstance::GetIMView(int32 mode)
{
	BView *view = new MyContainerView();
	return view;
}

JapaneseIMContentInstance::JapaneseIMContentInstance(Content *content, GHandler *handler,
													 const BMessage& params)
	: ContentInstance(content, handler),
	  fFrame(0.0f, 0.0f, 0.0f, 0.0f), fView(NULL)
{
	fView = GetIMView(0);

	if (fView) {
		float w, h;
		fView->GetPreferredSize(&w, &h);
		fView->ResizeTo(w, h);
	}
}

JapaneseIMContentInstance::~JapaneseIMContentInstance()
{
	if (fView) {
		BWindow *win = fView->Window();
		if (win && win->Lock()) {
			fView->RemoveSelf();
			win->Unlock();
		}
		delete fView;
	}
}

status_t JapaneseIMContentInstance::GetSize(int32 *width, int32 *height, uint32 *flags)
{
	*width = 200;
	*height = 70;
	*flags = STRETCH_HORIZONTAL;
	return B_OK;
//	return ContentInstance::GetSize(width, height, flags);
}

status_t JapaneseIMContentInstance::FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight)
{
	fFrame = newFrame;
	if (fView != NULL) {
		fView->MoveTo(fFrame.LeftTop());
		fView->ResizeTo(fFrame.Width(), fFrame.Height());
	}
	return B_OK;
}

status_t JapaneseIMContentInstance::AttachedToView(BView *view, uint32 */*contentFlags*/)
{
	PRINT(("JapaneseIMContentInstance: Attaching to %p\n", view));
	if (fView != NULL) {
		view->AddChild(fView);
	}
	return B_OK;
}

status_t JapaneseIMContentInstance::DetachedFromView()
{
	PRINT(("JapaneseIMContentInstance: Detaching from view.\n"));

	if (fView != NULL) {
		fView->RemoveSelf();
	}
	return B_OK;
}
	
// ---------------------- JapaneseIMContent ----------------------

JapaneseIMContent::JapaneseIMContent(void* handle)
	: Content(handle)
{
}

JapaneseIMContent::~JapaneseIMContent()
{
}

bool JapaneseIMContent::IsInitialized()
{
	return true;
}

size_t JapaneseIMContent::GetMemoryUsage()
{
	return 0;
}

ssize_t JapaneseIMContent::Feed(const void *d, ssize_t count, bool done)
{
	(void)d;
	(void)count;
	(void)done;

	return B_OK;
}

status_t JapaneseIMContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage& attributes)
{
	BMessage params;
	attributes.FindMessage("paramTags", &params);
	*outInstance = new JapaneseIMContentInstance(this, handler, params);
	return B_OK;
}

// ----------------------- JapaneseIMContentFactory -----------------------


void JapaneseIMContentFactory::GetIdentifiers(BMessage* into)
{
	 /*
	 ** BE AWARE: Any changes you make to these identifiers should
	 ** also be made in the 'addattr' command in the makefile.
	 */
	into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.Be.JapaneseIM");
}
	
Content* JapaneseIMContentFactory::CreateContent(void* handle,
												 const char* mime,
												 const char* extension)
{
	(void)mime;
	(void)extension;
	return new JapaneseIMContent(handle);
}

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if (n == 0) return new JapaneseIMContentFactory;
	return 0;
}
