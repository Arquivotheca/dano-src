/*******************************************************************************
/
/	File:			Menu.h
/
/   Description:    BMenu display a menu of selectable items.
/
/	Copyright 1994-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _MENU_H
#define _MENU_H

#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <View.h>

/*----------------------------------------------------------------*/
/*----- Menu decalrations and structures -------------------------*/

class BMenuItem;
class BMenuBar;
class BMenuWindow;
class BMenuFrame;
class BMenu;

class _ExtraMenuData_;

extern "C" status_t _init_interface_kit_();

enum menu_layout {
	B_ITEMS_IN_ROW = 0,
	B_ITEMS_IN_COLUMN,
	B_ITEMS_IN_MATRIX
};

struct menu_info {
	float		font_size;
	font_family	f_family;
	font_style	f_style;
	rgb_color	background_color;
	int32		separator;
	bool		click_to_open;
	bool		triggers_always_shown;
};

status_t	set_menu_info(menu_info *info);
status_t	get_menu_info(menu_info *info);

typedef bool (* menu_tracking_hook )(BMenu *, void *);

/*----------------------------------------------------------------*/
/*----- BMenu class ----------------------------------------------*/

class BMenu : public BView
{
public:
						BMenu(	const char *title,
								menu_layout layout = B_ITEMS_IN_COLUMN);
						BMenu(const char *title, float width, float height);
virtual					~BMenu();

						BMenu(BMessage *data);
static	BArchivable		*Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;

virtual void			AttachedToWindow();
virtual void			DetachedFromWindow();

		bool			AddItem(BMenuItem *item);
		bool			AddItem(BMenuItem *item, int32 index);
		bool			AddItem(BMenuItem *item, BRect frame);
		bool			AddItem(BMenu *menu);
		bool			AddItem(BMenu *menu, int32 index);
		bool			AddItem(BMenu *menu, BRect frame);
		bool			AddList(BList *list, int32 index);
		bool			AddSeparatorItem();
		bool			RemoveItem(BMenuItem *item);
		BMenuItem		*RemoveItem(int32 index);
		bool			RemoveItems(int32 index,
									int32 count,
									bool del = false);
		bool			RemoveItem(BMenu *menu);

		BMenuItem		*ItemAt(int32 index) const;
		BMenu			*SubmenuAt(int32 index) const;
		int32			CountItems() const;
		int32			IndexOf(BMenuItem *item) const;
		int32			IndexOf(BMenu *menu) const;
		BMenuItem		*FindItem(uint32 command) const;
		BMenuItem		*FindItem(const char *name) const;

virtual	status_t		SetTargetForItems(BHandler *target);
virtual	status_t		SetTargetForItems(BMessenger messenger);
virtual	void			SetEnabled(bool state);
virtual	void			SetRadioMode(bool state);
virtual	void			SetTriggersEnabled(bool state);
virtual void			SetMaxContentWidth(float max);

		void			SetLabelFromMarked(bool on);
		bool			IsLabelFromMarked();
		bool			IsEnabled() const;	
		bool			IsRadioMode() const;
		bool			AreTriggersEnabled() const;
		bool			IsRedrawAfterSticky() const;
		float			MaxContentWidth() const;

		BMenuItem		*FindMarked();

		BMenu			*Supermenu() const;
		BMenuItem		*Superitem() const;

virtual void			MessageReceived(BMessage *msg);
virtual	void			KeyDown(const char *bytes, int32 numBytes);
virtual void			Draw(BRect updateRect);
virtual void			SetFont(const BFont *font, uint32 mask = B_FONT_ALL);
virtual void			GetPreferredSize(float *width, float *height);
virtual void			ResizeToPreferred();
virtual	void			FrameMoved(BPoint new_position);
virtual	void			FrameResized(float new_width, float new_height);
		void			InvalidateLayout();
	
virtual BHandler		*ResolveSpecifier(BMessage *msg,
										int32 index,
										BMessage *specifier,
										int32 form,
										const char *property);
virtual status_t		GetSupportedSuites(BMessage *data);

virtual status_t		Perform(perform_code d, void *arg);

virtual void		MakeFocus(bool state = true);
virtual void		AllAttached();
virtual void		AllDetached();

		void		SetItemMargins(	float left, 
									float top, 
									float right, 
									float bottom);
		void		GetItemMargins(	float *left, 
									float *top, 
									float *right, 
									float *bottom) const;

		void		GetFrameMargins(float *left, 
									float *top, 
									float *right, 
									float *bottom) const;
									
protected:
				
					BMenu(	BRect frame,
							const char *viewName,
							uint32 resizeMask,
							uint32 flags,
							menu_layout layout,
							bool resizeToFit);

virtual	BPoint		ScreenLocation();

		menu_layout	Layout() const;

virtual	void		Show();
		void		Show(bool selectFirstItem);
		void		Hide();
		BMenuItem	*Track(	bool start_opened = false,
							BRect *special_rect = NULL);
	
public:
		enum add_state {
			B_INITIAL_ADD,
			B_PROCESSING,
			B_ABORT
		};
virtual	bool		AddDynamicItem(add_state s);
virtual void		DrawBackground(BRect update);

		void		SetTrackingHook(menu_tracking_hook func, void *state);
		
virtual void		AdjustSubmenuLocation(BPoint* inout_location,
										  BMenu* submenu);

/*----- Private or reserved -----------------------------------------*/
private:
friend class BWindow;
friend class BMenuBar;
friend class BMenuItem;
friend class BMenuFrame;
friend status_t	set_menu_info(menu_info *);
friend status_t	get_menu_info(menu_info *);
friend void		_b_cache_menu_info(const BMessage& src);

virtual	void			_ReservedMenu4();
virtual	void			_ReservedMenu5();
virtual	void			_ReservedMenu6();

		BMenu			&operator=(const BMenu &);

		void		InitData(BMessage *data = NULL);
		bool		_show(bool selectFirstItem = false);
		void		_hide();
		BMenuItem	*_track(int *action, long start = -1);
		bool		_AddItem(BMenuItem *item, int32 index);
		bool		RemoveItems(int32 index,
								int32 count,
								BMenuItem *item,
								bool del = false);
		void		LayoutItems(int32 index);
		void		ComputeLayout(int32 index, bool bestFit, bool moveItems,
								  float* width, float* height);
		BRect		Bump(BRect current, BPoint extent, int32 index) const;
		BPoint		ItemLocInRect(BRect frame) const;
		BRect		CalcFrame(BPoint where, bool *scrollOn);
		bool		ScrollMenu(BRect bounds, BPoint loc, bool *fast);
		void		ScrollIntoView(BMenuItem *item);

		void		DrawItems(BRect updateRect);
		int			State(BMenuItem **item = NULL) const;
		void		InvokeItem(BMenuItem *item, bool now = false);

		bool		OverSuper(BPoint loc);
		bool		OverSubmenu(BMenuItem *item, BPoint loc);
		BMenuWindow	*MenuWindow();
		void		DeleteMenuWindow();
		BMenuItem	*HitTestItems(BPoint where, BPoint slop = B_ORIGIN) const;
		BRect		Superbounds() const;
		void		CacheFontInfo();

		void		ItemMarked(BMenuItem *item);
		void		Install(BWindow *target);
		void		Uninstall();
		void		SelectItem(	BMenuItem *m,
								uint32 showSubmenu = 0,
								bool selectFirstItem = false);
		BMenuItem	*CurrentSelection() const;
		bool		SelectNextItem(BMenuItem *item, bool forward);
		BMenuItem	*NextItem(BMenuItem *item, bool forward) const;
		bool		IsItemVisible(BMenuItem *item) const;
		void		SetIgnoreHidden(bool on);
		void		SetStickyMode(bool on);
		bool		IsStickyMode() const;
		void		CalcTriggers();
		const char	*ChooseTrigger(const char *title, BList *chars);
		void		UpdateWindowViewSize(bool upWind = true);
		bool		IsStickyPrefOn();
		void		RedrawAfterSticky(BRect bounds);
		bool		OkToProceed(BMenuItem *);

		status_t	ParseMsg(BMessage *msg, int32 *sindex, BMessage *spec,
						int32 *form, const char **prop,
						BMenu **tmenu, BMenuItem **titem, int32 *user_data,
						BMessage *reply) const;

		status_t	DoMenuMsg(BMenuItem **next, BMenu *tar, BMessage *m,
						BMessage *r, BMessage *spec, int32 f) const;
		status_t	DoMenuItemMsg(BMenuItem **next, BMenu *tar, BMessage *m,
						BMessage *r, BMessage *spec, int32 f) const;

		status_t	DoEnabledMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
						BMessage *r) const;
		status_t	DoLabelMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
						BMessage *r) const;
		status_t	DoMarkMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
						BMessage *r) const;
		status_t	DoDeleteMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
						BMessage *r) const;
		status_t	DoCreateMsg(BMenuItem *ti, BMenu *tm, BMessage *m,
						BMessage *r, bool menu) const;

		bool 		DrawSnakeSelection() const;
		bool		DrawNewLook() const;
		void 		ClipDrawSnakeCommon(BView *, bool);
		void 		DrawItemDirect(BRect);
		void 		ClipBackgroundSnake(BView *);
		void 		DrawBackgroundSnake(BView *);

		struct p_menu_info : public menu_info {
			menu_info	info;
			BFont		item_font;
			bool		zsnake;
		};
		
static	p_menu_info	sMenuInfo;
static	bool		sSwapped;

		BMenuItem	*fChosenItem;
		BList		fItems;
		BRect		fPad;
		BMenuItem	*fSelected;
		BMenuWindow	*fCachedMenuWindow;
		BMenu		*fSuper;
		BMenuItem	*fSuperitem;
		BRect		fSuperbounds;
		float		fAscent;
		float		fDescent;
		float		fFontHeight;
		uint32		fState;
		menu_layout	fLayout;
		BRect		*fExtraRect;
		float		fMaxContentWidth;
		BPoint		*fInitMatrixSize;
		uint32		fDrawItemClipWindow;	// temporarily here, move it to space saved
											// by stuffing fLayout and fState in the same word
											
		_ExtraMenuData_	*fExtraMenuData;	// !!
		uint32		_reserved[2];

		char		fTrigger;
		char		_reserved_char[3];
		bool		fResizeToFit : 1;
		bool		fUseCachedMenuLayout : 1;
		bool		fEnabled : 1;
		bool		fDynamicName : 1;
		bool		fRadioMode : 1;
		bool		fTrackNewBounds : 1;
		bool		fStickyMode : 1;
		bool		fIgnoreHidden : 1;
		bool		fTriggerEnabled : 1;
		bool		fRedrawAfterSticky : 1;
		bool		fAttachAborted : 1;
		bool		fKeyPressed : 1;
		bool		fSkipSnakeDraw : 1;
		int			_reserved_bool : 19;
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _MENU_H */
