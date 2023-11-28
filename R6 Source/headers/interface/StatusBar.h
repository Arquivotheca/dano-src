/*******************************************************************************
/
/	File:			StatusBar.h
/
/   Description:    BStatusBar displays a "percentage-of-completion" gauge.
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_STATUS_BAR_H
#define	_STATUS_BAR_H

#include <BeBuild.h>
#include <String.h>
#include <View.h>

/*----------------------------------------------------------------*/
/*----- BStatusBar class -----------------------------------------*/

class BStatusBar : public BView {

public:
					BStatusBar(	BRect frame,
								const char *name,
								const char *label = NULL,
								const char *trailing_label = NULL);
					BStatusBar(BMessage *data);
virtual				~BStatusBar();
static	BArchivable	*Instantiate(BMessage *data);
virtual	status_t	Archive(BMessage *data, bool deep = true) const;

virtual	void		AttachedToWindow();
virtual	void		MessageReceived(BMessage *msg);
virtual	void		Draw(BRect updateRect);

virtual	void		SetBarColor(rgb_color color);
virtual	void		SetBarHeight(float height);
virtual	void		SetText(const char *str);
virtual	void		SetTrailingText(const char *str);
virtual	void		SetMaxValue(float max);

virtual	void		Update(	float delta,
							const char *main_text = NULL,
							const char *trailing_text = NULL);
virtual	void		Reset(	const char *label = NULL,
							const char *trailing_label = NULL);
virtual	void		SetTo(	float value,
							const char *main_text = NULL,
							const char *trailing_text = NULL);

		float		CurrentValue() const;
		float		MaxValue() const;
		rgb_color	BarColor() const;
		float		BarHeight() const;
		const char	*Text() const;
		const char	*TrailingText() const;
		const char	*Label() const;
		const char	*TrailingLabel() const;

virtual	void		MouseDown(BPoint pt);
virtual	void		MouseUp(BPoint pt);
virtual	void		WindowActivated(bool state);
virtual	void		MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
virtual	void		DetachedFromWindow();
virtual	void		FrameMoved(BPoint new_position);
virtual	void		FrameResized(float new_width, float new_height);

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

//virtual	void		_ReservedStatusBar1();
virtual	void		_ReservedStatusBar2();
virtual	void		_ReservedStatusBar3();
virtual	void		_ReservedStatusBar4();

		BStatusBar	&operator=(const BStatusBar &);

		void		InitObject(const char *l, const char *aux_l);
		void		Resize();
		bool		SetAndInvalidate(BString* into, const char* text,
									 float* width, float pos, bool trailing,
									 const font_height* fh = 0);
		void		InvalidateLabel(float left, float width,
									const font_height* fh = 0);
		
		float		LabelPos() const;
		float		TextPos() const;
		float		TrailingTextPos() const;
		float		TrailingLabelPos() const;
		float		TextBaseline(const font_height& fh) const;
		BRect		BarRect(const BRect& bounds, const font_height& fh) const;
		float		BarPos(const BRect& bar_rect, float value) const;
		void		_Draw(BRect updateRect);

		BString		fLabel;
		BString		fTrailingLabel;
		BString		fText;
		BString		fTrailingText;
		float		fMax;
		float		fCurrent;
		float		fBarHeight;
		float		fLabelWidth;
		float		fTrailingLabelWidth;
		float		fTextWidth;
		float		fTrailingTextWidth;
		rgb_color	fBarColor;
		bool		fCustomBarHeight;
		bool		_pad1;
		bool		_pad2;
		bool		_pad3;
		uint32		_reserved[2];		// was 4
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _STATUS_BAR_H */
