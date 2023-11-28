/*******************************************************************************
/
/	File:			StringView.h
/
/   Description:    BStringView draw a non-editable text string
/
/	Copyright 1992-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _STRING_VIEW_H
#define _STRING_VIEW_H

#include <BeBuild.h>
#include <Font.h>		// for truncate flags
#include <String.h>
#include <View.h>

/*----------------------------------------------------------------*/
/*----- BStringView class ----------------------------------------*/

class BStringView : public BView
{

public:
					BStringView(BRect bounds,
								const char *name, 
								const char *text,
								uint32 resizeFlags =
									B_FOLLOW_LEFT | B_FOLLOW_TOP,
								uint32 flags = B_WILL_DRAW);
					BStringView(BRect bounds,
								const char *name, 
								const char *text,
								uint32 truncation_mode,
								uint32 resizeFlags,
								uint32 flags);
					BStringView(BMessage *data);
virtual 			~BStringView();
static	BArchivable	*Instantiate(BMessage *data);
virtual	status_t	Archive(BMessage *data, bool deep = true) const;

		void		SetText(const char *text);
		const char	*Text() const;
		void		SetAlignment(alignment flag);
		alignment	Alignment() const;
		void		SetTruncation(uint32 mode);
		void		ClearTruncation();
		uint32		Truncation();
		bool		HasTruncation();
		
virtual	void		AttachedToWindow();
virtual	void		Draw(BRect bounds);

virtual void		MessageReceived(BMessage *msg);
virtual	void		MouseDown(BPoint pt);
virtual	void		MouseUp(BPoint pt);
virtual	void		MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
virtual	void		DetachedFromWindow();
virtual	void		FrameMoved(BPoint new_position);
virtual	void		FrameResized(float new_width, float new_height);
virtual void		SetFont(const BFont *font, uint32 mask = B_FONT_ALL);

virtual BHandler	*ResolveSpecifier(BMessage *msg,
									int32 index,
									BMessage *specifier,
									int32 form,
									const char *property);

virtual void		ResizeToPreferred();
virtual void		GetPreferredSize(float *width, float *height);
virtual void		MakeFocus(bool state = true);
virtual void		AllAttached();
virtual void		AllDetached();
virtual status_t	GetSupportedSuites(BMessage *data);


/*----- Private or reserved -----------------------------------------*/
virtual status_t	Perform(perform_code d, void *arg);

private:

		const char*	GetShownText();
		
virtual	void		_ReservedStringView1();
virtual	void		_ReservedStringView2();
virtual	void		_ReservedStringView3();

		BStringView	&operator=(const BStringView &);
		
		BString		fText;
		alignment	fAlign;
		uint16		fTruncationMode;	// was _reserved[0]
		bool		fValidTruncation;
		bool		_reservedBool1;
		BString		fTruncatedText;		// was _reserved[1]
		uint32		_reserved[1];		// was 3
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _STRING_VIEW_H */
