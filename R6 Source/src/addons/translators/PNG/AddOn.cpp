/*--------------------------------------------------------------------*\
  File:      AddOn.cpp
  Creator:   Matt Bogosian <mattb@be.com>
  Copyright: (c)1998, Be, Inc. All rights reserved.
  Description: Source file containing a translator for PNG images.
      Note: the comments in this file are meant to aid in the
      illustration of use (via example) of the translator protocol
      described in the BE BOOK (<http://www.be.com/documentation/
      be_book/The%20Translation%20Kit/Translator%20Protocol.html>).
      You may want to familiarize yourself with the protocol before
      delving into this code.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include "AddOn.h"
#include "ConfigView.h"
#include "PNGTranslator.h"
#include "message_archive.h"

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <TranslatorAddOn.h>

#include <Application.h>
#include <Window.h>
#include <Alert.h>
#include <stdlib.h>


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-= Definitions, Enums, Typedefs, Consts =-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

// Translator protocol stuff
static const size_t k_in_png1_ind(0);
static const size_t k_in_png2_ind(k_in_png1_ind + 1);
static const size_t k_in_bitmap_ind(k_in_png2_ind + 1);
static const size_t k_in_null_ind(k_in_bitmap_ind + 1);

static const size_t k_out_png1_ind(0);
static const size_t k_out_bitmap_ind(k_out_png1_ind + 1);
static const size_t k_out_null_ind(k_out_bitmap_ind + 1);

// Configuration stuff
static const char * const k_config_file_name("PNGTranslatorSettings");

// Writing configuration stuff
const char * const k_config_intrlcng("Interlacing Type");
const char * const k_config_res_x("X Resolution");
const char * const k_config_res_y("Y Resolution");
const char * const k_config_res_units("Resolution Units");
const char * const k_config_offset_x("X Offset");
const char * const k_config_offset_y("Y Offset");
const char * const k_config_offset_units("Offset Units");

const int32 k_default_intrlcng(PNG_INTERLACE_ADAM7);
const size_t k_default_res_x(4000);
const size_t k_default_res_y(4000);
const int32 k_default_res_units(PNG_RESOLUTION_METER);
const size_t k_default_offset_x(0);
const size_t k_default_offset_y(0);
const int32 k_default_offset_units(PNG_OFFSET_PIXEL);


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=- Function Prototypes =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*\
  Function name: static copyFormatToInfo
  Defined in:    PNGAddOn.cpp
  Arguments:     const translation_format * const a_src - the
                     translation format.
                 translator_info * const a_dest - the translator info.
                 const translator_id a_id - the translator ID.
                     Default: 0.
  Returns:       none
  Throws:        none
  Description: Function to copy the information from a translation
      format struct to a translator information struct.
\*--------------------------------------------------------------------*/

static void copyFormatToInfo(const translation_format * const a_src, translator_info * const a_dest, const translator_id a_id = 0);

/*--------------------------------------------------------------------*\
  Function name: static isTranslatable
  Defined in:    PNGAddOn.cpp
  Arguments:     const uint32 a_src_type - the known type of the
                     source data stream.
                 const uint32 a_dest_type - the known type of the
                     destination data stream.
  Returns:       bool - true if the combination is translatable with
                     this translator, false otherwise.
  Throws:        none
  Description: Function to determine if one known type may be
      translated into another known type with this translator.
\*--------------------------------------------------------------------*/

static bool isTranslatable(const uint32 a_src_type, const uint32 a_dest_type);


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-= Global Variables =-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

// Define the required globals
char translatorName[] = "PNG Images";
char translatorInfo[100];
//int32 translatorVersion(PNG_LIBPNG_VER);
int32 translatorVersion = B_BEOS_VERSION;

namespace BPrivate {
class infoFiller {
public:
	infoFiller()
		{
			sprintf(translatorInfo, "PNG image translator v%d.%d.%d, %s",
				translatorVersion>>8, (translatorVersion>>4)&0xf, translatorVersion&0xf,
				__DATE__);
		}
};
static infoFiller theFiller;
}

// Define the optional globals
translation_format inputFormats[] = {
	{ B_PNG_FORMAT,	B_TRANSLATOR_BITMAP, 0.6, 0.2, "image/png", "PNG image" },
	{ B_PNG_FORMAT, B_TRANSLATOR_BITMAP, 0.6, 0.2, "image/x-png", "PNG image" },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.5, 0.2, "image/x-be-bitmap", "Be Bitmap image" },
	{ 0, 0, 0, 0, 0, 0	}
};

translation_format outputFormats[] = {
	inputFormats[k_in_png1_ind],
	inputFormats[k_in_bitmap_ind],
	inputFormats[k_in_null_ind]
};


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
inline static void copyFormatToInfo(const translation_format * const a_src, translator_info * const a_dest, const translator_id a_id)
//====================================================================
{
	a_dest->type = a_src->type;
	a_dest->group = a_src->group;
	a_dest->quality = a_src->quality;
	a_dest->capability = a_src->capability;
	strncpy(a_dest->MIME, a_src->MIME, 251);
	strncpy(a_dest->name, a_src->name, 251);
	a_dest->translator = a_id;
}

//====================================================================
static bool isTranslatable(const uint32 a_src_type, const uint32 a_dest_type)
//====================================================================
{
	if (a_src_type != B_TRANSLATOR_ANY_TYPE
		&& a_src_type == a_dest_type)
	{
		return true;
	}
	
	if (a_src_type == B_TRANSLATOR_BITMAP)
	{
		return (a_dest_type == 0
			|| a_dest_type == B_PNG_FORMAT);
	}
	
	if (a_src_type == B_PNG_FORMAT)
	{
		return (a_dest_type == 0
			|| a_dest_type == B_TRANSLATOR_BITMAP);
	}
	
	return false;
}

//====================================================================
status_t Identify(BPositionIO * const a_src, const translation_format * const /*a_src_fmt*/, BMessage * const a_io_ext, translator_info * const a_dest_info, const uint32 a_dest_type)
//====================================================================
{
	// Make a guess (don't trust a_src_fmt)
	Translator *trnsltr(NULL);
	uint32 src_type(B_TRANSLATOR_ANY_TYPE);
	
	// Check to see if the source is a PNG image
	if (src_type == 0
		&& (trnsltr = new BitmapToPNGTranslator(a_src, NULL, a_io_ext)) != NULL
		&& trnsltr->Status() == B_NO_ERROR)
	{
		src_type = B_TRANSLATOR_BITMAP;
	}
	
	delete trnsltr;
	trnsltr = NULL;
	
	if (src_type == 0
		&& (trnsltr = new PNGToBitmapTranslator(a_src, NULL)) != NULL
		&& trnsltr->Status() == B_NO_ERROR)
	{
		src_type = B_PNG_FORMAT;
	}
	
	delete trnsltr;
	trnsltr = NULL;
	
	// Check to see if we can translate the requested types
	if (!isTranslatable(src_type, a_dest_type))
	{
		return B_NO_TRANSLATOR;
	}
	
	// Find the correct format for the specified destination type
	size_t i(0);
	
	while (inputFormats[i].type != src_type)
	{
		if (inputFormats[i++].type == 0)
		{
				return B_NO_TRANSLATOR;
		}
	}
	
	copyFormatToInfo(&inputFormats[i], a_dest_info);
	
	return B_NO_ERROR;
}

//====================================================================
status_t Translate(BPositionIO * const a_src, const translator_info * const a_src_info, BMessage * const a_io_ext, const uint32 a_dest_type, BPositionIO * const a_dest)
//====================================================================
{
	if (a_src_info == NULL)
	{
		return B_NO_TRANSLATOR;
	}
	
	// Default to something which makes sense
	uint32 dest_type(a_dest_type);
	// The spec requires this
	if (dest_type == 0) dest_type = B_TRANSLATOR_BITMAP;
	
	// Try to find a translator
	Translator *trnsltr(NULL);
	
	if (a_src_info->type == B_PNG_FORMAT
		&& dest_type == B_TRANSLATOR_BITMAP)
	{
		if ((trnsltr = new PNGToBitmapTranslator(a_src, a_dest)) == NULL)
		{
			return B_NO_MEMORY;
		}
	}
	else if (a_src_info->type == B_TRANSLATOR_BITMAP
		&& dest_type == B_PNG_FORMAT)
	{
		if ((trnsltr = new BitmapToPNGTranslator(a_src, a_dest, a_io_ext)) == NULL)
		{
			return B_NO_MEMORY;
		}
	}
	else if (a_src_info->type == dest_type)
	{
		if ((trnsltr = new DumbCopyTranslator(a_src, a_dest)) == NULL)
		{
			return B_NO_MEMORY;
		}
	}
	
	// Translate it bizniotch!
	status_t err(B_NO_TRANSLATOR);
	
	if (trnsltr != NULL
		&& trnsltr->Status(&err) == B_NO_ERROR)
	{
		err = trnsltr->Translate();
	}
	
	delete trnsltr;
	
	return err;
}

//====================================================================
status_t MakeConfig(BMessage * const /*a_io_ext*/, BView ** const a_out_view, BRect * const a_out_extent)
//====================================================================
{
	status_t err;
//#ifdef ARCHIVE_VIEWS
//	char add_on_path[B_PATH_NAME_LENGTH + 1];
//	add_on_path[B_PATH_NAME_LENGTH] = '\0';
//#ifdef USE_ATTRIBUTES_INSTEAD_OF_RESOURCES
//	BNode add_on_node;
//#else
//	BFile add_on_file;
//	BResources add_on_rsrcs;
//#endif
//	BMessage msg;
//#endif
//	*a_out_view = NULL;
//	
//#ifdef ARCHIVE_VIEWS
//	// Try to instantiate the view from the attribute
//	if ((err = findImagePath(add_on_path)) == B_NO_ERROR
//#ifdef USE_ATTRIBUTES_INSTEAD_OF_RESOURCES
//		&& (err = add_on_node.SetTo(add_on_path)) == B_NO_ERROR
//		&& (err = getMessageFromAttribute(&add_on_node, &msg, ConfigView::mk_name_config_view)) == B_NO_ERROR
//#else
//		&& (err = add_on_file.SetTo(add_on_path, B_READ_ONLY)) == B_NO_ERROR
//		&& (err = add_on_rsrcs.SetTo(&add_on_file)) == B_NO_ERROR
//		&& (err = getMessageFromResources(&add_on_rsrcs, &msg, ConfigView::mk_name_config_view)) == B_NO_ERROR
//#endif
//		&& (*a_out_view = new ConfigView(&msg, &err)) == NULL)
//	{
//		err = B_NO_MEMORY;
//	}
//	// Try to instantiate the view from the code
//	else if (err != B_NO_ERROR)
//#endif
//	{
//		delete *a_out_view;
		
		a_out_extent->Set(0,0,239,239);
		if ((*a_out_view = new ConfigView(*a_out_extent, &err)) == NULL)
		{
			err = B_NO_MEMORY;
		}
		else (*a_out_view)->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
//	}
	
	// Clean up on failure
	if (err != B_NO_ERROR)
	{
		delete *a_out_view;
		*a_out_view = NULL;
	}
	
	return err;
}

//====================================================================
status_t GetConfigMessage(BMessage * const a_config_msg)
//====================================================================
{
	a_config_msg->MakeEmpty();
	a_config_msg->what = B_ENDORSABLE;
	
	// Try to read the settings file
	BPath config_path;
	BFile config;
	
	if ((find_directory(B_USER_SETTINGS_DIRECTORY, &config_path) == B_NO_ERROR
			&& config_path.Append(k_config_file_name) == B_NO_ERROR
			&& config.SetTo(config_path.Path(), B_READ_ONLY) == B_NO_ERROR)
		|| (find_directory(B_COMMON_SETTINGS_DIRECTORY, &config_path) == B_NO_ERROR
			&& config_path.Append(k_config_file_name) == B_NO_ERROR
			&& config.SetTo(config_path.Path(), B_READ_ONLY) == B_NO_ERROR))
	{
		a_config_msg->Unflatten(&config);
	}
	
	// Add the defaults if no settings exist
	if (!a_config_msg->HasInt32(k_config_intrlcng))
	{
		a_config_msg->AddInt32(k_config_intrlcng, k_default_intrlcng);
	}
	
	if (!a_config_msg->HasInt32(k_config_res_x))
	{
		a_config_msg->AddInt32(k_config_res_x, k_default_res_x);
	}
	
	if (!a_config_msg->HasInt32(k_config_res_y))
	{
		a_config_msg->AddInt32(k_config_res_y, k_default_res_y);
	}
	
	if (!a_config_msg->HasInt32(k_config_res_units))
	{
		a_config_msg->AddInt32(k_config_res_units, k_default_res_units);
	}
	
	if (!a_config_msg->HasInt32(k_config_offset_x))
	{
		a_config_msg->AddInt32(k_config_offset_x, k_default_offset_x);
	}
	
	if (!a_config_msg->HasInt32(k_config_offset_y))
	{
		a_config_msg->AddInt32(k_config_offset_y, k_default_offset_y);
	}
	
	if (!a_config_msg->HasInt32(k_config_offset_units))
	{
		a_config_msg->AddInt32(k_config_offset_units, k_default_offset_units);
	}
	
	return B_NO_ERROR;
}

//====================================================================
status_t saveConfiguration(const BMessage * const a_config_msg)
//====================================================================
{
	// Try to read the settings file
	status_t err;
	BPath config_path;
	BFile config;
	
	if (((err = find_directory(B_USER_SETTINGS_DIRECTORY, &config_path)) == B_NO_ERROR
			&& (err = config_path.Append(k_config_file_name)) == B_NO_ERROR
			&& (err = config.SetTo(config_path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE)) == B_NO_ERROR)
		|| ((err = find_directory(B_COMMON_SETTINGS_DIRECTORY, &config_path)) == B_NO_ERROR
			&& (err = config_path.Append(k_config_file_name)) == B_NO_ERROR
			&& (err = config.SetTo(config_path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE)) == B_NO_ERROR))
	{
		return a_config_msg->Flatten(&config);
	}
	
	return err;
}

// Make it an app

class TranslatorWindow : public BWindow {
	public:
		TranslatorWindow(BRect rect, const char *name, window_type kind, uint32 flags);
		bool QuitRequested();
};

TranslatorWindow::TranslatorWindow(BRect rect, const char *name, window_type kind, uint32 flags) :
	BWindow(rect, name, kind, flags) {

}

bool TranslatorWindow::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

const char *translator_file_name = "PNGTranslator";
const char *translator_window_title = "PNG Settings";

int main() {
	char app_signature[255];
	sprintf(app_signature, "application/x-vnd.Be-%s", translator_file_name);
	BApplication app(app_signature);
	
	BRect extent(0, 0, 239, 239);
	BRect window_rect(extent);
	window_rect.OffsetTo(100, 100);
	TranslatorWindow *window = new TranslatorWindow(window_rect, translator_window_title,
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE);

	BView *config = NULL;
	status_t err = MakeConfig(NULL, &config, &extent);
	if ((err < B_OK) || (config == NULL)) {
		char error_message[255];
		sprintf(error_message, "%s does not currently allow user configuration.", translator_file_name);
		BAlert *alert = new BAlert("No Config", error_message, "Quit");
		alert->Go();
		exit(1);
	}
	
	window->ResizeTo(extent.Width(), extent.Height());
	window->AddChild(config);
	window->Show();
	app.Run();
	return 0;
}

