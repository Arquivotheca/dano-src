/* ++++++++++
	FILE:	ArchiveDefs.h
	Written By:	Peter Potrebic
	DATE:	Fri Oct 25 08:50:40 PDT 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _ARCHIVE_DEFS_H
#define _ARCHIVE_DEFS_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <message_strings.h>

extern rgb_color	_long_to_color_(ulong color);
extern ulong		_color_to_long_(rgb_color rgb);

// New-style names
#define BS_NAME				"be:name"
#define BS_BOUNDS			"be:bounds"
#define BS_POSITION			"be:position"
#define BS_SCALE			"be:scale"
#define BS_VIEW_FLAGS		"be:viewflags"
#define BS_VIEWS			"be:views"
#define BS_EVENT_MASK		"be:eventmask"
#define BS_EVENT_OPTIONS	"be:eventopts"
#define BS_DOUBLE_BUFFERING	"be:doublebuf"
#define BS_ENCODING			"be:encoding"
#define BS_AUTO_FILL		"be:autofill"

// Common
#define S_NAME				"_name"
#define S_FRAME				"_frame"
#define S_TYPE				"_type"
#define S_TEXT				"_text"
#define	S_FLAGS				"_flags"
#define S_HIDDEN			"be:hidden"
#define S_DISABLED			"_disable"
#define S_LABEL				"_label"
#define S_SELECTED			"_sel"
#define S_VALUE				"_val"
#define S_MESSAGE			"_msg"
#define	S_ALT_MESSAGE		"_2nd_msg"
#define S_DATA				"_data"
#define S_STYLE				"_style"
#define	S_DEFAULT			"_default"
#define S_LAYOUT			"_layout"
#define S_VERSION			"_ver"
#define S_ENDIANESS			"_endian"
#define S_TOOL_TIP_TEXT		"be:tooltiptext"

// BHandler

// BWindow
#define S_TITLE				"_title"
#define S_ZOOM_LIMITS		"_zoom"
#define S_SIZE_LIMITS		"_sizel"
#define S_PULSE_RATE		"_pulse"
#define S_TITLE				"_title"
#define S_WLOOK				"_wlook"
#define S_WFEEL				"_wfeel"
#define S_WORKSPACES		"_wspace"

// BView
#define S_RESIZE_MODE		"_resize_mode"
#define S_DRAW_MODE			"_dmod"
#define S_PEN_SIZE			"_psize"
#define S_PEN_LOCATION		"_ploc"
#define S_ORIGIN_LOCATION	"_origin"
#define S_LINE_MODE_CAPJOIN	"_lmcapjoin"
#define S_LINE_MODE_MITER	"_lmmiter"
#define S_BLENDING_MODE		"_blend"
#define S_FONT_FAMILY_STYLE	"_fname"
#define S_FONT_FLOATS		"_fflt"
#define S_COLORS			"_color"
#define	S_VIEWS				"_views"
#define S_ALIGN				"_align"
#define	S_MAX				"_max"
#define S_EVENT_MASK		"_evmsk"
#define S_DOUBLE_BUFFERING	"_dbuf"

// BMenu
#define S_RESIZE_TO_FIT		"_rsize_to_fit"
#define S_DISABLE_TRIGGERS	"_trig_disabled"
#define S_RADIO_MODE		"_radio"
#define S_LABEL_FROM_MARKED	"_dyn_label"
#define S_MENU_ITEMS		"_items"
#define S_ITEM_FRAMES		"_i_frames"

// BControl
#define S_WANTS_NAVIGATION	"be:wants_nav"

// BTextControl
#define S_OFFSCREEN	"_use_off"
#define	S_CELL_SIZE	"_csize"

// BTextControl
#define S_ALIGN_LABEL		"_a_label"
#define S_ALIGN_TEXT		"_a_text"
#define S_DIVIDER			"_divide"
#define S_MOD_MESSAGE		"_mod_msg"

// BPictureButton
#define S_ENABLED_ON	"_e_on"
#define S_ENABLED_OFF	"_e_off"
#define S_DISABLED_ON	"_d_on"
#define S_DISABLED_OFF	"_d_off"
#define S_BEHAVIOR		"_behave"

// BAlert
#define S_ALERT_TYPE		"_atype"
#define S_BUTTON_KEYS		"_but_key"
#define S_BUTTON_WIDTH		"_but_width"

// BBitmap
#define	S_COLOR_SPACE		"_cspace"
#define S_ACCEPTS_VIEWS		"_view_ok"
#define S_NEED_CONTIGUOUS	"_contiguous"
#define S_BITMAP_FLAGS		"_bmflags"
#define S_ROW_BYTES			"_rowbytes"
#define S_SCREEN_ID			"_screenid"

// BListView
#define S_LIST_VIEW_TYPE	"_lv_type"
#define	S_LIST_ITEMS		"_l_items"

// BListView
#define	S_FULL_LIST_ITEMS	"_l_full_items"

// BListItem
#define S_EXPANDED			"_li_expanded"
#define S_OUTLINE_LEVEL		"_li_outline_level"

// BScrollBar and BScrollView
#define S_RANGE				"_range"
#define S_STEPS				"_steps"
#define S_ORIENTATION		"_orient"
#define S_PROPORTION		"_prop"

// BSlider
#define S_MINLABEL			"_minlbl"
#define S_MAXLABEL			"_maxlbl"
#define S_MIN				"_min"
#define S_MAX				"_max"
#define S_INCVALUE			"_incrementvalue"
#define S_HASHMARKCOUNT		"_hashcount"
#define S_HASHMARKS			"_hashloc"	
#define S_THUMBSTYLE		"_sstyle"
#define S_DELAY 			"_sdelay"
#define S_FILL_COLOR 		"_fcolor"
#define S_BAR_THICKNESS		"_bthickness"

// BMenuField
// BPopUpMenu
// BPicture

// BStatusBar
#define S_HEIGHT			"_high"
#define	S_BAR_COLOR			"_bcolor"
#define	S_TRAILING_TEXT		"_ttext"
#define	S_TRAILING_LABEL	"_tlabel"

// BTabView
#define S_VIEW_LIST			"_view_list"

// BTextView
#define	S_TAB_WIDTH			"_tab"
#define	S_AUTO_INDENT		"_auto_in"
#define	S_WORD_WRAP			"_wrap"
#define	S_TEXT_RECT			"_trect"
#define	S_NOT_SELECTABLE	"_nsel"
#define	S_NOT_EDITABLE		"_nedit"
#define	S_SELECTION			"_sel"
#define	S_DISALLOWED_CHARS	"_dis_ch"
#define S_STYLABLE			"_stylable"
#define S_COLORSPACE		"_col_sp"
#define S_RUNS				"_runs"

// BShelf
#define S_ALLOW_ZOMBIES		"_zom_alw"
#define S_DISPLAY_ZOMBIES	"_zom_dsp"
#define S_GENERATION_COUNT	"_sg_cnt"
#define S_UID				"_s_uid"

#endif
