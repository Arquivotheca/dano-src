/*--------------------------------------------------------------------*\
  File:      ConfigView.cpp
  Creator:   Matt Bogosian <mattb@be.com>
  Copyright: (c)1998, Be, Inc. All rights reserved.
  Description: Source file containing the configuration classes for
      the PNG image translator add-on.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include "ConfigView.h"
#include "AddOn.h"
#include "CycleStringView.h"
#include "message_archive.h"

#include "png.h"

#include <AppFileInfo.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <TextControl.h>
#include <TranslatorAddOn.h>
#include <TranslatorFormats.h>
#include <Window.h>

#include <stdio.h>
#include <ctype.h>


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-= Definitions, Enums, Typedefs, Consts =-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

// Initialize the ConfigView public static const data members
const char * const ConfigView::mk_name_config_view("PNG:View");

// Initialize the ConfigView private static const data members
const uint32 ConfigView::mk_msg_intrlcng('pngM');

const ConfigView::KeyValue ConfigView::mk_vals_intrlcng[] =
{
	{
		"None",
		PNG_INTERLACE_NONE
	},
	
	{
		"Adam7",
		PNG_INTERLACE_ADAM7
	},
	
	{
		NULL,
		0
	}
};

const float ConfigView::mk_min_height(24.0);
const float ConfigView::mk_spacer(5.0);
const char * const ConfigView::mk_name("Name");
const char * const ConfigView::mk_long_vers("Long Version");

const char * const ConfigView::mk_name_views[] =
{
	mk_name,
	mk_long_vers,
	NULL,
	k_config_intrlcng,
	NULL,
	NULL
};

const char * const ConfigView::mk_name_menu_fields[] =
{
	k_config_intrlcng,
	NULL
};


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
ConfigView::ConfigView(const BRect a_frame, status_t * const a_err) :
//====================================================================
	Inherited(a_frame, mk_name_config_view, B_FOLLOW_ALL, B_WILL_DRAW)
{
	Init();
	
	if ((m_status = GetConfigMessage(&m_config_msg)) != B_NO_ERROR)
	{
		Status(a_err);
		return;
	}
	
	BMenuField *menu_field;

	font_height fh;
	be_bold_font->GetHeight(&fh);
	BRect r(10, 15, 10 + be_bold_font->StringWidth(translatorName), 15 + fh.ascent + fh.descent);

	BStringView *str = new BStringView(r, "title", translatorName);
	str->SetFont(be_bold_font);
	AddChild(str);
	
	char versStr[100];
	sprintf(versStr, "v%d.%d.%d %s", (int)(translatorVersion>>8), (int)((translatorVersion>>4)&0xf),
		(int)((translatorVersion)&0xf), __DATE__);
	r.top = r.bottom + 20;
	be_plain_font->GetHeight(&fh);
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(versStr);
	
	str = new BStringView(r, "info", versStr);
	str->SetFont(be_plain_font);
	AddChild(str);
	
	char *copyright_string = "© 1998-1999 Be Incorporated";
	r.top = r.bottom + 10;
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(copyright_string);
	
	CycleStringView *by_view;
	char *strs[5];
	strs[0] = copyright_string;
	strs[1] = "by Matt Bogosian";
	strs[2] = copyright_string;
	strs[3] = B_UTF8_OPEN_QUOTE "...make like a circus seal..." B_UTF8_CLOSE_QUOTE;
	strs[4] = NULL;
	
	if ((by_view = new CycleStringView(r, mk_long_vers, strs, B_FOLLOW_NONE, B_WILL_DRAW, NULL)) == NULL)
	{
		m_status = B_NO_MEMORY;
		Status(a_err);
		return;
	}
	
	if ((m_status = by_view->Status()) != B_NO_ERROR)
	{
		delete by_view;
		Status(a_err);
		return;
	}
	
	by_view->SetFont(be_plain_font);
	AddChild(by_view);
	
//	char img_path[B_PATH_NAME_LENGTH + 1];
//	img_path[0] = img_path[B_PATH_NAME_LENGTH] = '\0';
//	findImagePath(img_path);
//	BFile img_file;
//	BAppFileInfo img_file_info;
//	version_info info;
//	memset(&info, 0, sizeof(info));
//	BRect bounds(0.0, 0.0, 0.0, 0.0);
//	
//	if (img_file.SetTo(img_path, O_RDONLY) == B_NO_ERROR
//		&& img_file_info.SetTo(&img_file) == B_NO_ERROR
//		&& img_file_info.GetVersionInfo(&info, B_APP_VERSION_KIND) == B_NO_ERROR)
//	{
//		CycleStringView *by_view;
//		char *strs[5];
//		strs[0] = info.long_info;
//		strs[1] = "by Matt Bogosian";
//		strs[2] = info.long_info;
//		strs[3] = B_UTF8_OPEN_QUOTE "...make like a circus seal..." B_UTF8_CLOSE_QUOTE;
//		strs[4] = NULL;
//		
//		if ((by_view = new CycleStringView(bounds, mk_long_vers, strs, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP)) == NULL)
//		{
//			m_status = B_NO_MEMORY;
//			Status(a_err);
//			return;
//		}
//		
//		if ((m_status = by_view->Status()) != B_NO_ERROR)
//		{
//			delete by_view;
//			Status(a_err);
//			return;
//		}
//		
//		by_view->SetAlignment(B_ALIGN_CENTER);
//		AddChild(by_view);
//	}
	
	int32 intrlcng;
	
	if (m_config_msg.FindInt32(k_config_intrlcng, &intrlcng) != B_NO_ERROR)
	{
		intrlcng = k_default_intrlcng;
	}
	
	if ((menu_field = CreateMenuField(k_config_intrlcng, mk_msg_intrlcng, mk_vals_intrlcng, intrlcng, r.bottom + 10)) == NULL)
	{
		m_status = B_NO_MEMORY;
		Status(a_err);
		return;
	}
	
	AddChild(menu_field);
	Status(a_err);
}

//====================================================================
ConfigView::~ConfigView(void)
//====================================================================
{
#ifdef ARCHIVE_VIEWS
	char add_on_path[B_PATH_NAME_LENGTH + 1];
	add_on_path[B_PATH_NAME_LENGTH] = '\0';
#ifdef USE_ATTRIBUTES_INSTEAD_OF_RESOURCES
	BNode add_on_node;
#else
	BFile add_on_file;
	BResources add_on_rsrcs;
#endif
	BMessage msg;
	
	// Try to store the view as an attribute (if it isn't already)
	if (m_status == B_NO_ERROR
		&& findImagePath(add_on_path) == B_NO_ERROR
#ifdef USE_ATTRIBUTES_INSTEAD_OF_RESOURCES
		&& add_on_node.SetTo(add_on_path) == B_NO_ERROR
		&& getMessageFromAttribute(&add_on_node, &msg, mk_name_config_view) != B_NO_ERROR
#else
		&& add_on_file.SetTo(add_on_path, B_READ_WRITE) == B_NO_ERROR
		&& add_on_rsrcs.SetTo(&add_on_file) == B_NO_ERROR
		&& getMessageFromResources(&add_on_rsrcs, &msg, mk_name_config_view) != B_NO_ERROR
#endif
		&& Archive(&msg) == B_NO_ERROR)
	{
#ifdef USE_ATTRIBUTES_INSTEAD_OF_RESOURCES
		setAttributeFromMessage(&add_on_node, &msg, mk_name_config_view);
#else
		setResourceFromMessage(&add_on_rsrcs, &msg, mk_name_config_view);
#endif
	}
#endif
}

//====================================================================
void ConfigView::AttachedToWindow(void)
//====================================================================
{
	Inherited::AttachedToWindow();
	
	// Arrange the troops on the battlefield
	size_t i(0);
//	BView *view;
//	BRect frame((Parent() != NULL) ? Parent()->Bounds() : Window()->Bounds());
//	float width, height;
//	bool found_null(false);
//	frame.top = mk_spacer;
//	frame.left += mk_spacer * 2.0;
//	frame.right -= mk_spacer * 2.0;
//	
//	while (true)
//	{
//		// Make sure we have something to move
//		if (mk_name_views[i] != NULL
//			&& (view = FindView(mk_name_views[i])) != NULL)
//		{
//			found_null = false;
//			view->GetPreferredSize(&width, &height);
//			
//			// Thank you Peter for your wonderfully accurate
//			// implemenations of GetPreferredSize()
//			if (height < mk_min_height)
//			{
//				height = mk_min_height;
//			}
//			
//			frame.bottom = frame.top + height;
//			view->MoveTo(frame.left, frame.top);
//			view->ResizeTo(frame.right - frame.left, frame.bottom - frame.top);
//			frame.top = frame.bottom + mk_spacer;
//		}
//		// Wait for two nulls in a row
//		else
//		{
//			if (found_null)
//			{
//				break;
//			}
//			
//			found_null = true;
//			frame.top = frame.bottom + mk_min_height;
//		}
//		
//		i++;
//	}
	
	// Set the targets appropriately
	BMenuField *menu_field;
	i = 0;
	
	while ((menu_field = dynamic_cast<BMenuField *>(FindView(mk_name_menu_fields[i++]))) != NULL)
	{
		BMenu *menu;
		
		if ((menu = menu_field->Menu()) == NULL)
		{
			continue;
		}
		
		BMenuItem *menu_item;
		int32 i(0);
		
		while ((menu_item = menu->ItemAt(i++)) != NULL)
		{
			menu_item->SetTarget(this);
		}
		
	}
}

//====================================================================
void ConfigView::Draw(BRect a_update_rect)
//====================================================================
{
	Inherited::Draw(a_update_rect);
}

//====================================================================
void ConfigView::MessageReceived(BMessage * const a_msg)
//====================================================================
{
	// Take care of it
	if (a_msg->what == mk_msg_intrlcng)
	{
		int32 intrlcng;
		
		if (a_msg->FindInt32(k_config_intrlcng, &intrlcng) == B_NO_ERROR)
		{
			m_config_msg.ReplaceInt32(k_config_intrlcng, intrlcng);
			saveConfiguration(&m_config_msg);
		}
	}
	else
	{
		Inherited::MessageReceived(a_msg);
	}
}

//====================================================================
status_t ConfigView::Status(status_t * const a_err) const
//====================================================================
{
	if (a_err != NULL)
	{
		*a_err = m_status;
	}
	
	return m_status;
}

//====================================================================
void ConfigView::Init(void)
//====================================================================
{
	m_status = B_NO_ERROR;
}

//====================================================================
BMenuField *ConfigView::CreateMenuField(const char * const a_name, const uint32 a_msg_type, const KeyValue * const a_vals, const int32 a_dflt_val, int top)
//====================================================================
{
	//BRect bounds(10,100,230,124);
	BRect bounds;
	bounds.left = 10;
	bounds.top = top;
	bounds.bottom = bounds.top + 24;
	bounds.right = bounds.left + be_plain_font->StringWidth(a_name) +
		be_plain_font->StringWidth(a_vals[0].m_key) + 20 + 32;
	
	BMenu *menu;
	BMenuField *menu_field(NULL);
	
	if ((menu = new BMenu("")) == NULL
		|| (menu_field = new BMenuField(bounds, a_name, a_name, menu, (uint32)B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE)) == NULL)
	{
		delete menu;
		return NULL;
	}
	
	menu_field->SetDivider(be_plain_font->StringWidth(a_name) + 7);
	menu->SetLabelFromMarked(true);
	menu->SetRadioMode(true);
	//menu_field->SetAlignment(B_ALIGN_RIGHT);
	size_t i(0);
	
	while (a_vals[i].m_key != NULL)
	{
		BMessage *msg;
		BMenuItem *menu_item(NULL);
		
		if ((msg = new BMessage(a_msg_type)) == NULL
			|| (menu_item = new BMenuItem(a_vals[i].m_key, msg)) == NULL)
		{
			delete msg;
			delete menu_field;
			return NULL;
		}
		
		if (msg->AddInt32(a_name, a_vals[i].m_val) != B_NO_ERROR)
		{
			delete menu_item;
			delete menu_field;
			return NULL;
		}
		
		if (!menu->AddItem(menu_item))
		{
			delete menu_item;
			delete menu_field;
			return NULL;
		}
		
		if (i == 0
			|| a_vals[i].m_val == a_dflt_val)
		{
			menu_item->SetMarked(true);
		}
		
		i++;
	}
	
	return menu_field;
}
