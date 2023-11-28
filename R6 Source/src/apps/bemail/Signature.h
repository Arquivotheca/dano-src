//--------------------------------------------------------------------
//	
//	Signature.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <Alert.h>
#include <Beep.h>
#include <Box.h>
#include <FindDirectory.h>
#include <Font.h>
#include <fs_index.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <TextControl.h>
#include <Window.h>

const float kSigHeight = 200;
const float kSigWidth = 457;

/* it is assumed that StringLength(SIG_TEXT) > StringLength(NAME_TEXT) */
const char kNameText[] = "Title:";
const char kSigText[] = "Signature:";

#define INDEX_SIGNATURE		"_signature"

class	TMenu;
class	TNameControl;
class	TScrollView;
class	TSignatureView;
class	TSigTextView;

//====================================================================

class TSignatureWindow : public BWindow {
public:
				TSignatureWindow(BRect);
				~TSignatureWindow();
virtual void	MenusBeginning();
virtual void	MessageReceived(BMessage*);
virtual bool	QuitRequested();
virtual void	Show();
void 			FrameResized(float width, float height);
bool			Clear();
bool			IsDirty();
void			Save();

private:
	BMenuItem		*fCut;
	BMenuItem		*fCopy;
	BMenuItem		*fDelete;
	BMenuItem		*fNew;
	BMenuItem		*fPaste;
	BMenuItem		*fSave;
	BMenuItem		*fUndo;
	BEntry			fEntry;
	BFile			*fFile;
	TMenu			*fSignature;
	TSignatureView	*fSigView;
};

//--------------------------------------------------------------------

class TSignatureView : public BBox {
public:
				TSignatureView(BRect); 
virtual	void	AttachedToWindow();
TNameControl	*fName;
TSigTextView	*fTextView;

private:
float			fOffset;
};

//====================================================================


class TNameControl : public BTextControl {
public:
				TNameControl(BRect, const char*, BMessage*);
virtual void	AttachedToWindow();
virtual void	MessageReceived(BMessage*);

private:
char			fLabel[100];
};

//====================================================================

class TSigTextView : public BTextView {
public:
				TSigTextView(BRect, BRect); 
void			FrameResized(float width, float height);

virtual void	DeleteText(int32, int32);
virtual void	KeyDown(const char*, int32);
virtual void	InsertText(const char*, int32, int32, const text_run_array*);
virtual void	MessageReceived(BMessage*);

bool			fDirty;

private:
TSignatureView	*fParent;
};
#endif
