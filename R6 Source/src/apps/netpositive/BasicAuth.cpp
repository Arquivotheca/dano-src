// ===========================================================================
//	BasicAuth.cpp
//  Copyright 1998 by Be Incorporated.
// =============================================================================

#include "BasicAuth.h"
#include "NPApp.h"
#include "FolderWatcher.h"
#ifdef R4_COMPATIBLE
#include "Utils.h"
#endif
#include <Entry.h>
#include <View.h>
#include <Window.h>
#include <TextView.h>
#include <Button.h>
#include <CheckBox.h>
#include <Path.h>
#include <FindDirectory.h>
#include <E-mail.h>
#include <String.h>
#include <stdio.h>
#include <sys/param.h>
#include <malloc.h>

//=============================================================================
//	Only open one of these windows at a time

//=============================================================================

#define OKBUTTON 'okbu'
#define CANCELBUTTON 'cabu'
#define REMEMBER 'remb'

extern const char *kPasswordDialogPrompt;
extern const char *kOKButtonTitle;
extern const char *kCancelButtonTitle;
extern const char *kUsernamePrompt;
extern const char *kPasswordPrompt;
extern const char *kUserAndPassError;
extern const char *kAnonymousUserName;
extern const char *kAnonymousPassword;
extern const char *kRememberPasswordLabel;
extern const char *kRememberPasswordWarning;
extern const char *kPasswordFolderName;
extern const char *kPasswordMimeType;
extern const char *kApplicationSig;
extern const char *kPasswordFTPAttr;
extern const char *kPasswordUserAttr;
extern const char *kPasswordAuthAttr;
extern const char *kPasswordRealmAttr;

class TabTextView;

TLocker	sPasswordLock("Password lock");
BasicAuthFolder* gBasicAuthFolder;
static sem_id sInitSem = 0;


 class PasswordPanel : public BView {
public:
					PasswordPanel(BRect rect, const char* realm, BString& user, BString& auth, bool ftp, bool *save);
	virtual	void	AttachedToWindow();
	virtual	void	MessageReceived(BMessage *msg);
	virtual	void	Draw(BRect updateRect);

protected:
	BButton*		mOK;
	BButton*		mCancel;
	
	TabTextView*	mPassword;
	BTextView*		mUsername;
	BCheckBox*		mRememberPassword;
	
	bool			mFTP;
	BString& 		mUser;
	BString& 		mAuth;
	BString			mRealm;
	
	bool*			mSave;
};

//=============================================================================

class PasswordWindow : public BWindow {
public:
					PasswordWindow(BRect frame,const char* realm, BString& user, BString& auth, bool ftp, bool *save);
	virtual			~PasswordWindow();

	static	bool	Running();
	
protected:
	static	bool	mRunning;
};

bool PasswordWindow::mRunning = false;

//=============================================================================

PasswordWindow::PasswordWindow(BRect frame, const char* realm, BString& user, BString& auth, bool ftp, bool *save) : BWindow(frame, "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	frame.OffsetTo(0,0);
	PasswordPanel* p = new PasswordPanel(frame,realm,user,auth,ftp,save);
	AddChild(p);
	mRunning = true;
	Show();
}

PasswordWindow::~PasswordWindow()
{
	mRunning = false;
}

bool PasswordWindow::Running()
{
	return mRunning;
}

//=============================================================================
//	Text view that passes tabs to parent

class TabTextView : public BTextView {
public:
				TabTextView(BRect rect, char *name, BRect textRect, bool renderAsBullets);
virtual	void	KeyDown(const char *bytes, int32 numBytes);
virtual	void	InsertText(const char *inText, int32 inLength, int32 inOffset, const text_run_array *inRuns);
virtual	void	DeleteText(int32 fromOffset, int32 toOffset);

		const char *ActualText() {return mData.String();}

private:
		bool	mRenderAsBullets;
		BString	mData;
};

TabTextView::TabTextView(BRect rect, char *name, BRect textRect, bool renderAsBullets) : BTextView(rect,name,textRect,B_FOLLOW_LEFT_RIGHT,B_WILL_DRAW | B_NAVIGABLE), mRenderAsBullets(renderAsBullets)
{
}

void TabTextView::KeyDown(const char *bytes, int32 numBytes)
{
	switch (*bytes) {
		case B_TAB:
			BMessage msg(B_KEY_DOWN);
			msg.AddInt8("byte", B_TAB);
			msg.AddInt32("modifiers", B_OPTION_KEY);
			Window()->PostMessage(&msg);
			return;
	}
	BTextView::KeyDown(bytes, numBytes);
}

void TabTextView::InsertText(const char *inText, int32 inLength, int32 inOffset, const text_run_array *inRuns)
{
	uint32 unsignedOffset = inOffset;
	if (!mRenderAsBullets)
		BTextView::InsertText(inText, inLength, unsignedOffset, inRuns);
	else {
		BString before;
		before.SetTo(mData, MIN(inOffset, mData.Length()));
		BString after;
		BString inTextString;
		inTextString.SetTo(inText, inLength);
		
		if (inOffset < mData.Length())
			after = (mData.String()) + inOffset;
			
		mData = before;
		mData += inTextString;
		mData += after;
				
		BString newText;
		for (int i = 0; i < inLength; i++)
			newText += "*";

		BTextView::InsertText(newText.String(), inLength, inOffset, inRuns);
	}
}

void TabTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	uint32 unsignedFrom = fromOffset;
	uint32 unsignedTo = toOffset;
	
	BTextView::DeleteText(unsignedFrom, unsignedTo);
	if (mRenderAsBullets) {
		BString before;
		before.SetTo(mData, MIN(fromOffset, mData.Length()));
		BString after;
		if (toOffset < mData.Length())
			after = mData.String() + unsignedTo;

		mData = before;
		mData += after;
	}
}

//=============================================================================

PasswordPanel::PasswordPanel(BRect rect, const char* realm, BString& user, BString& auth, bool ftp, bool *save) : BView(rect, "PasswordPanel", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
	mFTP(ftp), mUser(user), mAuth(auth), mRealm(realm), mSave(save)
{
}

void PasswordPanel::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetFont(be_bold_font);
	SetViewColor(216,216,216);
	SetLowColor(216,216,216);
	
	int width = 80;		// Of buttons
	int height = 24;

	BRect r = Bounds();	// "Please enter the username and password for 'realm'"
	BRect text;
	r.InsetBy(10,5);
	r.bottom = r.top + 32;
	text = r;
	text.OffsetTo(B_ORIGIN);
	BTextView* t = new BTextView(r,"Text",text,B_FOLLOW_LEFT_RIGHT,B_WILL_DRAW);
	AddChild(t);
	t->SetWordWrap(true);
	t->MakeEditable(false);
	t->MakeSelectable(false);
	t->SetFont(be_bold_font);
	t->SetViewColor(216,216,216);
	t->SetLowColor(216,216,216);
	
	char prompt[1024];
	sprintf(prompt,kPasswordDialogPrompt, mRealm.String());
	t->SetText(prompt);
	
	r = Bounds();
	r.InsetBy(10,0);
	r.top += 32 + 8;
	r.left = r.right - (width + 8 + width);	// Align with buttons
	r.bottom = r.top + 13;
	
	text = r;
	text.OffsetTo(0, 0);
	text.InsetBy(1,1);
	mUsername = new TabTextView(r,"Username",text, false);
	AddChild(mUsername);
	
	r.OffsetBy(0,24);
	mPassword = new TabTextView(r,"Password",text, true);
	AddChild(mPassword);
	
	r.OffsetBy(0,20);
	r.left = 10;
	mRememberPassword = new BCheckBox(r, "Remember", kRememberPasswordLabel, new BMessage(REMEMBER));
	AddChild(mRememberPassword);
	
	r.OffsetBy(0,20);
	r.bottom += 20;
	text = r;
	text.OffsetTo(0,0);
	text.bottom += 20;
	t = new BTextView(r,"Text2", text, B_FOLLOW_LEFT_RIGHT,B_WILL_DRAW);
	AddChild(t);
	t->SetText(kRememberPasswordWarning);
	t->SetWordWrap(true);
	t->MakeEditable(false);
	t->MakeSelectable(false);
	t->SetFont(be_plain_font);
	t->SetViewColor(216,216,216);
	t->SetLowColor(216,216,216);
	
	
	r = BRect(0,0,width,height);
	r.OffsetTo(Bounds().right - (width + 8),Bounds().bottom - (height + 8));
	mOK = new BButton(r,"OK", kOKButtonTitle, new BMessage(OKBUTTON),B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(mOK);
	
	r.OffsetTo(Bounds().right - (width + 8 + width + 8),Bounds().bottom - (height + 8));
	mCancel = new BButton(r,"Cancel", kCancelButtonTitle, new BMessage(CANCELBUTTON),B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(mCancel);

	Window()->SetDefaultButton(mOK);
	mOK->SetTarget(this);
	mCancel->SetTarget(this);
	mUsername->MakeFocus(true);
}

void TextBevel(BView& view, BRect r);

void PasswordPanel::Draw(BRect)
{
	BRect r;
	const char *s;
	 
	r = mPassword->Frame();
	r.InsetBy(-1,-1);
	TextBevel(*this,r);
	s = kPasswordPrompt;
	DrawString(s,BPoint(r.left - (6 + StringWidth(s,strlen(s))),r.bottom - 2));
	
	r = mUsername->Frame();
	r.InsetBy(-1,-1);
	TextBevel(*this,r);
	s = kUsernamePrompt;
	DrawString(s,BPoint(r.left - (6 + StringWidth(s,strlen(s))),r.bottom - 2));
}

//=============================================================================
//	Base64 encoding

void Base64Str(uchar *src, int count, BString &encodedStr);

void
Base64Str(
	uchar	*src, 
	int		count,
	BString	&encodedStr)
{
	char *encStr = (char *)malloc(count * 2 + 64);
	count = encode_base64(encStr, (char *)src, count);
	encStr[count] = 0;
	encodedStr = encStr;
	free(encStr);
}

void PasswordPanel::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	
		case CANCELBUTTON:
			Window()->Quit();
			break;
	
//		Convert the username and password string into base64 encoded username:password

		case OKBUTTON: {
			char result[1024];
			const char *username = mUsername->Text();
			const char *password = mPassword->ActualText();
			
			if (!username) username = "";
			if (!password) password = "";
			
			*mSave = mRememberPassword->Value();
			
			if (mFTP) {
				mUser = username;
				mAuth = password;
				Window()->Quit();
			} else {
//				if (username && *username && password && *password) {
					sprintf(result,"%s:%s",mUsername->Text(),mPassword->ActualText());
					Base64Str((uchar*)result,strlen(result), mAuth);
					Window()->Quit();
//				} else
//					SimpleAlert(kUserAndPassError);	//€€€€€€€€€€€€€€€€€€€€ are they?
			}		
			break;
		}

		case B_TAB:
			if (mUsername->IsFocus()) {
				mPassword->MakeFocus();
				mPassword->SelectAll();
			} else {
				mUsername->MakeFocus();
				mUsername->SelectAll();
			}
			break;
		default:
			BView::MessageReceived(msg);
	}
}

//=============================================================================
//	Password window does not need to be a par of basic BasicAuth

bool BasicAuthFolder::RunPasswordWindow(const char *realm, BString& user, BString& auth, bool ftp, bool *save)
{
	sPasswordLock.Lock();
	BRect f(0,0,260,180);
	CenterWindowRect(&f);
	
	new PasswordWindow(f,realm,user,auth,ftp,save);
	while (PasswordWindow::Running())
		snooze(100000);
	sPasswordLock.Unlock();
	return auth.Length() != 0;
}

//=============================================================================
//	Get Authentication for a realm

const char*	BasicAuthFolder::Get(const char *realm)
{
	// Make sure the initialization is done.
	acquire_sem(sInitSem);
	release_sem(sInitSem);
	
//	Lookup realm, is there a BasicAuthItem object we can use?

	BasicAuthItem* ba;
	for (int i = 0; i < mItemList.CountItems(); i++) {
		BasicAuthItem *ba = (BasicAuthItem *)mItemList.ItemAt(i);
		if (strcmp(ba->Realm(),realm) == 0)
			return ba->Auth();
	}
			
//	Present a password window

	BString user;
	BString auth;
	bool save;
	if (RunPasswordWindow(realm,user,auth, false,&save) == false)
		return NULL;	// User cancelled
		
	ba = new BasicAuthItem(realm,user.String(),auth.String(), false, save);
	mItemList.AddItem(ba);
	return ba->Auth();		// Will be deleted if the password is bad
}

//	This password didn't work for a particular realm
//	Blow it out of the list and let the user try again

void BasicAuthFolder::Bad(const char *realm)
{
	for (int i = 0; i < mItemList.CountItems(); i++) {
		BasicAuthItem *ba = (BasicAuthItem *)mItemList.ItemAt(i);
		if (strcmp(ba->Realm(),realm) == 0) {
			Delete(ba);
			return;
		}
	}
}

void BasicAuthFolder::Good(const char *realm)
{
	for (int i = 0; i < mItemList.CountItems(); i++) {
		BasicAuthItem *ba = (BasicAuthItem *)mItemList.ItemAt(i);
		if (strcmp(ba->Realm(),realm) == 0) {
			if (ba->ShouldSave()) {
				BMessage msg;
				ba->Save(msg);
				AddFile(NULL, msg);
				ba->ShouldSave(false);
			}
			return;
		}
	}
}

//=============================================================================
//	Password's for ftp

bool BasicAuthFolder::GetFTP(const char *realm, char* user, char* pass, bool mustAsk)
{
	// Make sure the initialization is done.
	acquire_sem(sInitSem);
	release_sem(sInitSem);

//	Lookup realm, is there a BasicAuthItem object we can use?

	BasicAuthItem* ba;
	for (int i = 0; i < mItemList.CountItems(); i++) {
		BasicAuthItem *ba = (BasicAuthItem *)mItemList.ItemAt(i);
		if (strcmp(ba->Realm(),realm) == 0) {
			strcpy(user,ba->User());
			strcpy(pass,ba->Auth());
			return true;				// Found a password for this realm
		}
	}
	
	if (mustAsk == false) {				// Guess
		strcpy(user,kAnonymousUserName);
		strcpy(pass,kAnonymousPassword);
		return false;
	}
	
//	Present a password window

	BString userz;
	BString auth;
	bool save;
	if (RunPasswordWindow(realm,userz,auth,true, &save) == false)
		return false;	// User cancelled
		
	ba = new BasicAuthItem(realm,userz.String(),auth.String(),true, save);
	mItemList.AddItem(ba);
	strcpy(user,ba->User());
	strcpy(pass,ba->Auth());
	return true;		// Will be deleted if the password is bad
}

//=============================================================================
//=============================================================================

//CLinkedList BasicAuth::mItemList;
//FolderWatcher* BasicAuth::mPasswordFolder;

//	Remember the realm and the username:password in base64 format

 BasicAuthItem::BasicAuthItem(const char* realm, const char *user, const char* auth, bool ftp, bool save) :
	mFTP(ftp), mUser(user), mAuth(auth), mRealm(realm), mSave(save)
{
}

BasicAuthItem::BasicAuthItem() 
{
	mSave = false;
}


void BasicAuthItem::ReadMessageAttributes(BMessage *msg)
{
	mFTP = msg->FindBool(kPasswordFTPAttr);
	mUser = msg->FindString(kPasswordUserAttr);
	mAuth = msg->FindString(kPasswordAuthAttr);
	mRealm = msg->FindString(kPasswordRealmAttr);
}

FolderWatcher* BasicAuthFolder::CreateFolderWatcher()
{
	sInitSem = create_sem(0, "BasicAuth Init Locker");
	BPath settingsPath;
	find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath);
	settingsPath.Append(kPasswordFolderName);
	BEntry folderEntry(settingsPath.Path());
	gBasicAuthFolder = new BasicAuthFolder(folderEntry, kPasswordMimeType, kApplicationSig, be_app, false, true);

	thread_id tid = spawn_thread(InitThreadEntry, "BasicAuth init", B_NORMAL_PRIORITY + 2, NULL);
	resume_thread(tid);
	NetPositive::AddThread(tid);

	return gBasicAuthFolder;	
}

FolderItem* BasicAuthFolder::CreateFolderItem() const
{
	return new BasicAuthItem;
}


BasicAuthFolder::BasicAuthFolder(const BEntry& path,const char *fileType,const char *appSig,BLooper *looper,bool recursive,bool createDir) :
 FolderWatcher(path, fileType, appSig, looper, recursive, createDir, true) 
{
}


int32 BasicAuthFolder::InitThreadEntry(void *)
{
	gBasicAuthFolder->Init();
	release_sem_etc(sInitSem, 10, 0);
	NetPositive::RemoveThread(find_thread(NULL));
	return 0;
}
 
void BasicAuthItem::Save(BMessage &msg)
{
	msg.AddBool(kPasswordFTPAttr, mFTP);
	if (mUser.Length() > 0) msg.AddString(kPasswordUserAttr, mUser.String());
	if (mAuth.Length() > 0) msg.AddString(kPasswordAuthAttr, mAuth.String());
	if (mRealm.Length() > 0) msg.AddString(kPasswordRealmAttr, mRealm.String());
}

FolderItem* BasicAuthFolder::FindItem(const BMessage& attrs) const 
{
	bool FTP = attrs.FindBool(kPasswordFTPAttr);
	const char *user = attrs.FindString(kPasswordUserAttr);
	const char *auth = attrs.FindString(kPasswordAuthAttr);
	const char *realm = attrs.FindString(kPasswordRealmAttr);
	for (int i = 0; i < mItemList.CountItems(); i++) {
		BasicAuthItem *ba = (BasicAuthItem *)mItemList.ItemAt(i);
		if (ba->mFTP == FTP &&
			ba->mUser == user &&
			ba->mAuth == auth &&
			ba->mRealm == realm)
			return ba;
	}
	return NULL;
}

//	Nuke if the password didn't work

void BasicAuthFolder::Delete(BasicAuthItem *ba)
{
	if (ba->mFilename.Length())
	RemoveFile(ba->mFilename.String());
	
	mItemList.RemoveItem(ba);
}

const char*	BasicAuthItem::Realm()
{
	return mRealm.String();
}

const char*	BasicAuthItem::Auth()
{
	return mAuth.String();
}

const char*	BasicAuthItem::User()
{
	return mUser.String();
}
