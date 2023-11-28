/*******************************************************************************
/
/	File:			RulerView.h
/
/   Description:    BRulerView provides scrolling machinery for its contents
/                   with rulers (where the "contents" is some other view).
/
/	Copyright 2001, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_RULER_VIEW_H
#define	_RULER_VIEW_H

#include <BeBuild.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <View.h>

enum ruler_units
{
	B_INCHES = 0,
	B_CENTIMETERS,
	B_MILLIMETERS,
	B_PICAS,
	B_POINTS
};

class BDeadCorner;
class BRuler;


/* ---------------------------------------------------------------- */
/* ----- BRulerView class ----------------------------------------- */

class BRulerView : public BScrollView
{
	public:
							BRulerView			(const char* name,
												 BView* target,
												 uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
												 uint32 flags = 0,
												 bool horizontal_scroll_bar = false,
												 bool horizontal_ruler = false,
												 bool vertical_scroll_bar = false, 
												 bool vertical_ruler = false,
												 bool track_mouse = false,
												 ruler_units units = B_INCHES,
												 int32 divisions = 8,
												 float scale = 100.0,
												 border_style border = B_FANCY_BORDER);
							BRulerView			(BMessage* data);
		virtual				~BRulerView			();
		virtual	status_t	Archive				(BMessage* data,
												 bool deep = true) const;
		static	BArchivable*Instantiate			(BMessage* data);
		virtual	void		AllAttached			();
		virtual void		AllDetached			();
		virtual void		DrawCorner			(BDeadCorner* dead_corner,
												 BRect updateRect);
		virtual void		DrawRuler			(BRuler* ruler,
												 BRect updateRect);
		virtual	void		MouseMoved			(BPoint pt,
												 uint32 code,
												 const BMessage* msg);
		virtual void		RulerMenu			(BPoint location,
												 uint32 buttons);

		BDeadCorner*		Corner				();
		BRuler*				Ruler				(orientation);
		void				HideRuler			(orientation);
		void				ShowRuler			(orientation);
		const BFont*		Font				() const;
		void				SetFont				(const BFont* font);
		int32				Divisions			();
		void				SetDivisions		(int32 divisions);
		bool				MouseTracking		();
		void				SetMouseTracking	(bool track);
		float				OriginOffset		(orientation);
		void				SetOriginOffset		(orientation,
												 float offset);
		float				RulerOffset			(orientation);
		void				SetRulerOffset		(orientation,
												 float offset);
		ruler_units			RulerUnits			();
		void				SetRulerUnits		(ruler_units);
		float				Scale				();
		void				SetScale			(float);
		rgb_color			CornerFrameColor	() const;
		void				SetCornerFrameColor	(rgb_color color);
		rgb_color			FillColor			() const;
		void				SetFillColor		(rgb_color color);
		rgb_color			TextColor			() const;
		void				SetTextColor		(rgb_color color);


/* ----- Private or reserved -------------------------------------- */

	private:
		virtual	void		_ReservedRulerView01();
		virtual	void		_ReservedRulerView02();
		virtual	void		_ReservedRulerView03();
		virtual	void		_ReservedRulerView04();
		virtual	void		_ReservedRulerView05();
		virtual	void		_ReservedRulerView06();
		virtual	void		_ReservedRulerView07();
		virtual	void		_ReservedRulerView08();
		virtual	void		_ReservedRulerView09();
		virtual	void		_ReservedRulerView10();
		virtual	void		_ReservedRulerView11();
		virtual	void		_ReservedRulerView12();
		virtual	void		_ReservedRulerView13();
		virtual	void		_ReservedRulerView14();
		virtual	void		_ReservedRulerView15();
		virtual	void		_ReservedRulerView16();
		uint32				_reserved[16];

		void				Initialize			();
		void				UpdateRulers		();

		bool				fHorizontalRuler;
		bool				fVerticalRuler;
		bool				fTrackMouse;
		float				fHorizontalOriginOffset;
		float				fVerticalOriginOffset;
		float				fHorizontalRulerOffset;
		float				fVerticalRulerOffset;
		float				fScale;
		int32				fDivisions;
		ruler_units			fUnits;
		BFont				fFont;
		BPopUpMenu*			fMenu;
		BDeadCorner*		fCornerView;
		BRuler*				fHorizontalRulerView;
		BRuler*				fVerticalRulerView;
		rgb_color			fFillColor;
		rgb_color			fTextColor;
		rgb_color			fCornerFrameColor;
};


/* ---------------------------------------------------------------- */

class BRuler : public BView
{
	public:
							BRuler				(BRect	rect,
												 const char* name,
												 uint32 resizeMask,
												 uint32 flags,
												 orientation direction,
												 BRulerView*);
		void				Draw				(BRect updateRect);
		void				MessageReceived		(BMessage* msg);
		void				MouseDown			(BPoint pt);
		void				WindowActivated		(bool state);

		void				DrawRuler			(BRect updateRect);
		orientation			Orientation			();
		void				SetMouseMark		(float mark);

	private:
		void				CalculateMarks		(float unit_pixels,
												 int32* divisions,
												 int32* unit_mark,
												 int32* numeral_mark);

		bool				fActive;
		float				fMark;
		float				ScreenDPI;
		BRulerView*			fRulerView;
		orientation			fOrientation;
};


/* ---------------------------------------------------------------- */

class BDeadCorner : public BView
{
	public:
							BDeadCorner			(BRect	rect,
												 const char* name,
												 uint32 resizeMask,
												 uint32 flags,
												 BRulerView*);
		void				Draw				(BRect updateRect);
		void				MouseDown			(BPoint pt);

		void				DrawCorner			(BRect updateRect);

	private:
		BRulerView*			fRulerView;
};


/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#endif /* _RULER_VIEW_H */
