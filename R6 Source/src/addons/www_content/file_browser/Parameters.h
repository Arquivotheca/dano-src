/*-----------------------------------------------------------------*/
//
//	File:		Parameters.h
//
//	Written by:	Robert Polic
//
//	Copyright 2001, Be Incorporated
//
/*-----------------------------------------------------------------*/

#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

#include <Content.h>

using namespace Wagner;


/*-----------------------------------------------------------------*/
// labels:enum...
enum LABELS
{
	eMediaTabOrnament = 0,
	eMediaTabTitle,
	eMediaTabArtist,
	eMediaTabAlbum,
	eMediaTabTime,
	eMediaTabTrack,
	eMediaTabControls,
	eFileTabOrnament,
	eFileTabName,
	eFileTabModified,
	eFileTabSize,
	eFileTabControls,
	eLabelCount
};

// labels:param message names...
#define kMEDIA_TAB_ORNAMENT					"media_tab_ornament"
#define kMEDIA_TAB_TITLE					"media_tab_title"
#define kMEDIA_TAB_ARTIST					"media_tab_artist"
#define kMEDIA_TAB_ALBUM					"media_tab_album"
#define kMEDIA_TAB_TIME						"media_tab_time"
#define kMEDIA_TAB_TRACK					"media_tab_track"
#define kMEDIA_TAB_CONTROLS					"media_tab_controls"
#define kFILE_TAB_ORNAMENT					"file_tab_ornament"
#define kFILE_TAB_NAME						"file_tab_name"
#define kFILE_TAB_MODIFIED					"file_tab_modified"
#define kFILE_TAB_SIZE						"file_tab_size"
#define kFILE_TAB_CONTROLS					"file_tab_controls"

// labels:default values...
#define kDEFAULT_MEDIA_TAB_ORNAMENT			""
#define kDEFAULT_MEDIA_TAB_TITLE			"Title"
#define kDEFAULT_MEDIA_TAB_ARTIST			"Artist"
#define kDEFAULT_MEDIA_TAB_ALBUM			"Album"
#define kDEFAULT_MEDIA_TAB_TIME				"Time"
#define kDEFAULT_MEDIA_TAB_TRACK			"Track"
#define kDEFAULT_MEDIA_TAB_CONTROLS			""
#define kDEFAULT_FILE_TAB_ORNAMENT			""
#define kDEFAULT_FILE_TAB_NAME				"Name"
#define kDEFAULT_FILE_TAB_MODIFIED			"Modified"
#define kDEFAULT_FILE_TAB_SIZE				"Size"
#define kDEFAULT_FILE_TAB_CONTROLS			""


/*-----------------------------------------------------------------*/
// colors:enum...
enum COLORS
{
	eTabColor = 0,
	eTabSeperatorColor,
	eTabTextColor,
	eTabSortColor,
	eTabSortTextColor,
	eListColor,
	eListSeperatorColor,
	eListSelectedColor,
	eListTextColor,
	eListSelectedTextColor,
	eListSelectedFrameColor,
	eListRolloverColor,
	eListEditColor,
	eColorCount
};

// colors:param message names...
#define kTAB_COLOR							"tab_color"
#define kTAB_SEPERATOR_COLOR				"tab_seperator_color"
#define kTAB_TEXT_COLOR						"tab_text_color"
#define kTAB_SORT_COLOR						"tab_sort_color"
#define kTAB_SORT_TEXT_COLOR				"tab_sort_text"
#define kLIST_COLOR							"list_color"
#define kLIST_SEPERATOR_COLOR				"list_seperator_color"
#define kLIST_SELECTED_COLOR				"list_selected_color"
#define kLIST_TEXT_COLOR					"list_text_color"
#define kLIST_SELECTED_TEXT_COLOR			"list_selected_text_color"
#define kLIST_SELECTED_FRAME_COLOR			"list_selected_frame_color"
#define kLIST_ROLLOVER_COLOR				"list_rollover_color"
#define kLIST_EDIT_COLOR					"list_edit_color"

// colors:default values...
#define kDEFAULT_TAB_COLOR					{154, 158, 142, 255}
#define kDEFAULT_TAB_SEPERATOR_COLOR		{128, 132, 115, 255}
#define kDEFAULT_TAB_TEXT_COLOR				{  0,   0,   0, 255}
#define kDEFAULT_TAB_SORT_COLOR				{154, 158, 142, 255}
#define kDEFAULT_TAB_SORT_TEXT_COLOR		{  0,   0,   0, 255}
#define kDEFAULT_LIST_COLOR					{207, 207, 201, 255}
#define kDEFAULT_LIST_SEPERATOR_COLOR		{128, 132, 115, 255}
#define kDEFAULT_LIST_SELECTED_COLOR		{255, 255, 255, 255}
#define kDEFAULT_LIST_TEXT_COLOR			{ 14,  14,  13, 255}
#define kDEFAULT_LIST_SELECTED_TEXT_COLOR	{  0,   0,   0, 255}
#define kDEFAULT_LIST_SELECTED_FRAME_COLOR	{  0,   0,   0, 255}
#define kDEFAULT_LIST_ROLLOVER_COLOR		{144, 112, 144, 255}
#define kDEFAULT_LIST_EDIT_COLOR			{128, 132, 115, 255}


/*-----------------------------------------------------------------*/
// offsets:enum...
enum OFFSETS
{
	eTabHeight = 0,
	eTabSpacerWidth,
	eListSelectedFrameWidth,
	eListItemHeight,
	eListSpacerHeight,
	eOffsetsCount
};

// offsets:param message names...
#define kTAB_HEIGHT							"tab_height"
#define kTAB_SPACER_WIDTH					"tab_spacer_width"
#define kLIST_SELECTED_FRAME_WIDTH			"list_selected_frame_width"
#define kLIST_ITEM_HEIGHT					"list_item_height"
#define kLIST_SPACER_HEIGHT					"list_spacer_height"

// offsets:default values...
#define kDEFAULT_TAB_HEIGHT					21
#define kDEFAULT_TAB_SPACER_WIDTH			 1
#define kDEFAULT_LIST_SELECTED_FRAME_WIDTH	 1
#define kDEFAULT_LIST_ITEM_HEIGHT			23
#define kDEFAULT_LIST_SPACER_HEIGHT			 1


/*-----------------------------------------------------------------*/
// flags:enum...
enum FLAGS
{
	eSupportDrag = 0,
	eShowTrashedFiles,
	eFlagsCount
};

// flags:param message names...
#define kSUPPORT_DRAG						"support_drag"
#define kSHOW_TRASHED_FILES					"show_trashed_files"

// flags: default values...
#define kDEFAULT_SUPPORT_DRAG				true
#define kDEFAULT_SHOW_TRASHED_FILES			false


/*-----------------------------------------------------------------*/
// icons:enum...
enum ICONS
{
	eAudioIcon = 0,
	eVideoIcon,
	eImageIcon,
	eTextIcon,
	eHTMLIcon,
	eEmailIcon,
	eOtherIcon,
	eEditAcceptUpIcon,
	eEditAcceptOverIcon,
	eEditAcceptDownIcon,
	eEditRejectUpIcon,
	eEditRejectOverIcon,
	eEditRejectDownIcon,
	eSortAscendingIcon,
	eSortDescendingIcon,
	eIconCount
};

// icons:param message names...
#define kAUDIO_ICON							"audio_icon"
#define kVIDEO_ICON							"video_icon"
#define kIMAGE_ICON							"image_icon"
#define kTEXT_ICON							"text_icon"
#define kHTML_ICON							"html_icon"
#define kMAIL_ICON							"mail_icon"
#define kOTHER_ICON							"other_icon"
#define kEDIT_ACCEPT_UP_ICON				"approve_edit_up"
#define kEDIT_ACCEPT_OVER_ICON				"approve_edit_over"
#define kEDIT_ACCEPT_DOWN_ICON				"approve_edit_down"
#define kEDIT_REJECT_UP_ICON				"cancel_edit_up"
#define kEDIT_REJECT_OVER_ICON				"cancel_edit_over"
#define kEDIT_REJECT_DOWN_ICON				"cancel_edit_down"
#define kSORT_ASCENDING_ICON				"sort_ascending_icon"
#define kSORT_DESCENDING_ICON				"sort_descending_icon"

// icons:default values...
#define kTYPE_AUDIO							"file://$RESOURCES/Tray/images/type_audio.png"
#define kTYPE_VIDEO							"file://$RESOURCES/Tray/images/type_video.png"
#define kTYPE_IMAGE							"file://$RESOURCES/Tray/images/type_image.png"
#define kTYPE_TEXT							"file://$RESOURCES/Tray/images/type_other.png"
#define kTYPE_HTML							"file://$RESOURCES/Tray/images/type_html.png"
#define kTYPE_MAIL							"file://$RESOURCES/Tray/images/type_email.png"
#define kTYPE_OTHER							"file://$RESOURCES/Tray/images/type_other.png"
#define kAPPROVE_EDIT_UP					"file://$RESOURCES/Bookmarks/images/approve_edit_up.png"
#define kAPPROVE_EDIT_OVER					"file://$RESOURCES/Bookmarks/images/approve_edit_over.png"
#define kAPPROVE_EDIT_DOWN					"file://$RESOURCES/Bookmarks/images/approve_edit_down.png"
#define kCANCEL_EDIT_UP						"file://$RESOURCES/Bookmarks/images/cancel_edit_up.png"
#define kCANCEL_EDIT_OVER					"file://$RESOURCES/Bookmarks/images/cancel_edit_over.png"
#define kCANCEL_EDIT_DOWN					"file://$RESOURCES/Bookmarks/images/cancel_edit_down.png"
#define kSORT_ARROW							"file://$RESOURCES/Bookmarks/images/sort_arrow.png"


/*-----------------------------------------------------------------*/
// fonts:param message names...
#define kTAB_FONT							"tab_font"
#define kLIST_FONT							"list_font"

// fonts:default values...
#define kDEFAULT_TAB_FONT					be_bold_font
#define kDEFAULT_TAB_FONT_SIZE				12
#define kDEFAULT_LIST_FONT					be_bold_font
#define kDEFAULT_LIST_FONT_SIZE				12


/*-----------------------------------------------------------------*/
// misc:param message names...
#define kDEFAULT_VOLUME						"mounted_at"
#define kDEFAULT_VIEW						"view"


/*-----------------------------------------------------------------*/
// the all-encompassing struct
struct drawing_parameters
{
	BString				labels[eLabelCount];
	rgb_color			colors[eColorCount];
	int32				offsets[eOffsetsCount];
	bool				flags[eFlagsCount];
	ContentInstance*	icons[eIconCount];
	BFont				tab_font;
	BFont				list_font;
};
#endif
