/*******************************************************************************
/
/	File:			SlideRule.h
/
/   Description:    BSlideRule is a control which displays icons on a sliderule
/
/	Copyright 1993-99, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _SLIDE_RULE_H
#define _SLIDE_RULE_H

#include <BeBuild.h>
#include <Control.h>

class BBitmap;

/*----------------------------------------------------------------*/
/*----- BSlideRule defines -----------------------------------------*/

enum {
	B_SR_TOP_ARROW = 1,
	B_SR_BOTTOM_ARROW = 2,
	B_SR_RIGHT_ARROW = 4,
	B_SR_LEFT_ARROW = 8
};

/*----------------------------------------------------------------*/
/*----- BSlideRule class -------------------------------------------*/

class BSlideRule : public BControl {

public:
					BSlideRule(	BRect frame,
									const char *name,
									BMessage *message = NULL,
									uint32 arrows = B_SR_TOP_ARROW | B_SR_BOTTOM_ARROW,
									uint32 resizeMask = B_FOLLOW_TOP | B_FOLLOW_LEFT,
									uint32 flags = 0 );
		
virtual 			~BSlideRule();

					BSlideRule(BMessage *data);
static	BArchivable	*Instantiate(BMessage *data);
virtual	status_t	Archive(BMessage *data, bool deep = true) const;
	
virtual	void 		ResizeToPreferred();
virtual	void 		FrameResized(float new_width, float new_height);
virtual	void 		MakeFocus(bool focusState = true);
virtual	void		AttachedToWindow();
virtual	void		SetViewColor(rgb_color c);
virtual	void 		SetValue(int32 value); // Set icon without slide effect
					// int32 BControl::Value() Returns current icon
virtual void 		SetEnabled(bool on);
virtual	void 		GetPreferredSize(float *width, float *height);

					// App retains reponsibility for deleting icons
		int32 		AddIcon(const char *label, const BBitmap *largeIcon, const BBitmap *smallIcon);
		bool 		AddIcon(const char *label, const BBitmap *largeIcon, const BBitmap *smallIcon, int32 index);
		status_t 	RemoveIconAt(int32 icon);
		void 		RemoveAllIcons();
		int32 		CountIcons() const;

		void 		SlideToIcon(int32 icon);
		void 		SlideToNext();
		void 		SlideToPrevious();

		int32 		IconForPoint(BPoint where) const;
		BPoint 		IconOrigin(int32 icon) const ;

		void 		SetArrowPosition(float posistion); // 0.0 - 1.0
		float 		ArrowPosition() const;
		void		SetArrowMargin(float margin);
		float		ArrowMargin() const;
		void 		SetArrowSize(float size);
		float 		ArrowSize() const;
		void 		SetIconSpacing(float space);
		float 		IconSpacing(void);
		void		SetWrap(bool wrap);
		bool		Wrap(void);
/*----- Private or reserved -----------------------------------------*/
private:
virtual	void		_ReservedSlideRule1();
virtual	void		_ReservedSlideRule2();
virtual	void		_ReservedSlideRule3();
virtual	void		_ReservedSlideRule4();
virtual	void		_ReservedSlideRule5();
virtual	void		_ReservedSlideRule6();
virtual	void		_ReservedSlideRule7();
virtual	void		_ReservedSlideRule8();
virtual	void		_ReservedSlideRule9();
virtual	void		_ReservedSlideRule10();
virtual	void		_ReservedSlideRule11();
virtual	void		_ReservedSlideRule12();

		BView		*fChildPtr;
		uint32		_reserved[12];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
#endif /* _SLIDE_RULE_H */
