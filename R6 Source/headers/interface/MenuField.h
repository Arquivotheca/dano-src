/*******************************************************************************
/
/	File:			MenuField.h
/
/   Description:    BMenuField displays a labeled pop-up menu.
/
/	Copyright 1994-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#ifndef _MENU_FIELD_H
#define _MENU_FIELD_H

#include <BeBuild.h>
#include <Menu.h>		/* For convenience */
#include <String.h>
#include <View.h>

class BMenuBar;

namespace BPrivate {
class BMCMenuBar;
}

/*----------------------------------------------------------------*/
/*----- BMenuField class -----------------------------------------*/

class BMenuField : public BView
{
public:
						BMenuField(	BRect frame,
									const char *name,
									const char *label,
									BMenu *menu,
									uint32 resize = B_FOLLOW_LEFT|B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
						BMenuField(	BRect frame,
									const char *name,
									const char *label,
									BMenu *menu,
									bool fixed_size,
									uint32 resize = B_FOLLOW_LEFT|B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
						BMenuField(BMessage *data);
virtual					~BMenuField();
static	BArchivable		*Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;

virtual	void			Draw(BRect update);
virtual	void			AttachedToWindow();
virtual	void			AllAttached();
virtual	void			MouseDown(BPoint where);
virtual	void			KeyDown(const char *bytes, int32 numBytes);
virtual	void			MakeFocus(bool state);
virtual void			MessageReceived(BMessage *msg);
virtual void			WindowActivated(bool state);
virtual	void			MouseUp(BPoint pt);
virtual	void			MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
virtual	void			DetachedFromWindow();
virtual	void			AllDetached();
virtual	void			FrameMoved(BPoint new_position);
virtual	void			FrameResized(float new_width, float new_height);

		BMenu			*Menu() const;
		BMenuBar		*MenuBar() const;
		BMenuItem		*MenuItem() const;

virtual	void			SetLabel(const char *label);
		const char		*Label() const;
		
virtual void			SetEnabled(bool on);
		bool			IsEnabled() const;

virtual	void			SetAlignment(alignment label);
		alignment		Alignment() const;
virtual	void			SetDivider(float dividing_line);
		float			Divider() const;

		void			ShowPopUpMarker();
		void			HidePopUpMarker();

virtual BHandler		*ResolveSpecifier(BMessage *msg,
										int32 index,
										BMessage *specifier,
										int32 form,
										const char *property);
virtual status_t		GetSupportedSuites(BMessage *data);

virtual void            SetFont(const BFont *font, uint32 mask = B_FONT_ALL);

virtual void			ResizeToPreferred();
virtual void			GetPreferredSize(float *width, float *height);


/*----- Private or reserved -----------------------------------------*/
virtual status_t		Perform(perform_code d, void *arg);

private:
friend class BPrivate::BMCMenuBar;

virtual	void			_ReservedMenuField1();
virtual	void			_ReservedMenuField2();
virtual	void			_ReservedMenuField3();

		BMenuField		&operator=(const BMenuField &);


		void			InitObject(const char *label);
		void			InitObject2();
		void			DrawLabel(BRect bounds, BRect update);
		void			DistributeFont(const BFont* font = 0,
									   uint32 mask = B_FONT_ALL);
static	void			DistributeMenuFont(BMenu *menu, const BFont* font,
										   uint32 mask = B_FONT_ALL);
static	long			MenuTask(void *arg);
		void			LayoutViews(bool attach=false, bool initial=false);
		
		BRect			GetBorderFrame() const;
		
		bool			ExtendPartialMatch(const char* bytes, int32 numBytes=-1);
		BMenuItem*		FindPartialMatch(const char* prefix);
		void			ClearPartialMatch();
		
		char			*fLabel;
		BMenu			*fMenu;
		BPrivate::BMCMenuBar *fMenuBar;
		alignment		fAlign;
		float			fDivider;
		float			fStringWidth;
		bool			fEnabled;
		bool			fSelected;
		bool			fFixedSizeMB;
		bool			_reservedBool;
		thread_id		fMenuTaskID;
		bool			fPopUpMarker;	// was _reserved[0]
		bool			fHasLabel;
		BString			fPartialMatch;	// was _reserved[1]
		uint32			_reserved[1];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _MENU_FIELD_H */
