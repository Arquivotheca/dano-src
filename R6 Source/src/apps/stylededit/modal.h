//------------------------------------------------------------------
//
// A quick hack to give me some basic modal dialog functionality
//
// First cut: 11/10/93 	jdr
//------------------------------------------------------------------
#ifndef MODAL_H
#define MODAL_H 
 
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _TEXT_VIEW_H
#include <TextView.h>
#endif

class BCheckBox;

enum {
	cmdOK			= 'cYES',
	cmdCancel		= 'cCAN',
	cmdNo			= 'cNO ',
	cmdReplaceAll	= 'cRAL'
};


//--------------------------------------------------------------
class TMessageDialog : public BWindow {

public:
				TMessageDialog(	BRect bounds,
								const char *defaultLabel = "OK");
virtual	void	MessageReceived(BMessage* a_message);
		long	fPressed;

		BView	*Top();
		void	ReAddView(const char *name, bool make_default = FALSE);

private:
friend class TEntryDialog;
		short	fWidth;	// For convenience
		short 	fHeight;
		BView	*fTop;
};

//---------------------------------------------------------------
// This is a standard dialog that presents a text field for the user
// to type in, and has "OK" and "cancel" buttons
//---------------------------------------------------------------
class TEntryDialog : public TMessageDialog {

public:
				TEntryDialog(	BRect bounds,
								const char *message1,
								const char *initialText1,
								const char *message2 = NULL,
								const char *initialText2 = NULL,
								const char *defaultLabel = "OK",
								const char *cancelLabel = "Cancel",
								ulong max_chars1 = 0,
								bool numeric1 = FALSE);

	BTextView	*fText1;
	BTextView	*fText2;
};

class TFindReplaceDialog : public TEntryDialog {
public:
				TFindReplaceDialog(	BPoint topleft,
									const char *findStr,
									const char *replaceStr);
	
	void		MessageReceived(BMessage *msg);

	BCheckBox	*fBackwards;
	BCheckBox	*fWrap;
	BCheckBox	*fSensitive;
	BCheckBox	*fGlobalReplace;
};

class TSeparatorLine : public BView {
public:
							TSeparatorLine(BRect frame);

	virtual void			Draw(BRect updateRect);
};

#endif
