// ===========================================================================
//	NPApp.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#include "NPApp.h"
#include "HTMLWindow.h"
#include "URLView.h"
#include "HTMLDoc.h"
#include "Bookmarks.h"
#include "FindWindow.h"
#include "UResource.h"
#include "Protocols.h"	// Get rid of protocol stuff
#include "BeDrawPort.h"
#include "Cache.h"
#ifdef ADFILTER
#include "AdFilter.h"
#endif
#include "BasicAuth.h"
#include "FolderWatcher.h"
#include "FontSubstitution.h"
#include "Image.h"
#include "Cookie.h"
#include "HistoryMenu.h"
#include "URL.h"
#include "DownloadManager.h"
#include "Strings.h"
#include "NetPositive.h"
#ifdef JAVASCRIPT
#include "jseopt.h"
#include "jselib.h"
#endif
#include "MessageWindow.h"
#include "HTMLView.h"
#ifdef PLUGINS
#include "PluginSupport.h"
#endif

#include <Resources.h>
#include <FilePanel.h>
#include <Screen.h>
#include <Alert.h>
#include <NodeInfo.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <malloc.h>
#include <fs_attr.h>
#include <NodeMonitor.h>
#include <unistd.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <Path.h>
#include <Roster.h>
#include <Synth.h>
#include <MidiSynthFile.h>
#include <MidiPort.h>
#include <stdlib.h>
#include <WindowScreen.h>
#include <Autolock.h>
#include <TranslatorRoster.h>

bool NetPositive::mIsQuitting = false;


ConnectionManager *gConnectionMgr = NULL;
BTranslatorRoster *gTranslatorRoster = NULL;

static 		BString		sShortVersionString;
static		BString		sLongVersionString;
static		BString		sVersionNumber;
static		FolderWatcher *sGlobalFontSubFolder = NULL;
static		FolderWatcher *sGlobalBasicAuthFolder = NULL;
static		FolderWatcher *sGlobalHistoryFolder = NULL;
static		FolderWatcher *sGlobalBookmarkFolder = NULL;
static		color_space	sMainScreenColorSpace;


//=======================================================================
//=======================================================================
//	The instance of the platform object for be

bool gEZDebug = false;
bool gDumpData = false;
bool gCleanDump = false;

extern bool gDrawInfo;
extern bool gDrawCell;
extern bool gUseLatin5;

BMessage gPreferences;
entry_ref	gImageRef;

static BFilePanel *sOpenFilePanel;
entry_ref *sFilePanelDir;
BMessage* gPrintSettings = NULL;
const char *gWindowTitle;

extern const unsigned char gLinkCursor[] =	 {
	16,	/*cursor size, valid values are 16 and 32*/
	1,	/*1 bit per pixel (only value right now) */
	2,	/*hot spot vertical			 */
	2,	/*hot spot horizontal			 */

	/* data */
	0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
	0x08,0x01,	0x3c,0x21,	0x4c,0x71,	0x42,0x71,	0x30,0xf9,	0x0c,0xf9,	0x02,0x00,	0x01,0x00,
	
	/* mask */
	0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
	0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
};

extern const unsigned char gBusyCursor[kNumCursorPhases][68] = {
	{	// Phase 0
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0x50,0x10,
		0x50,0x10,	0x90,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0xff,0xf8,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0xf0,
		0x7f,0xf0,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8
	},
	{	// Phase 1
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0x50,0x10,
		0x50,0x10,	0x90,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0xf0,	0xa0,0x80,	0xa1,0x00,	0xff,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0xf0,
		0x7f,0xf0,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf0,	0xff,0x80,	0xff,0x00,	0xff,0x00
	},
	{	// Phase 2
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0x50,0x10,
		0x50,0x10,	0x90,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0xf0,	0x70,0x80,	0x0d,0x00,	0x03,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0xf0,
		0x7f,0xf0,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf0,	0x7f,0x80,	0x0f,0x00,	0x03,0x00
	},
	{	// Phase 3
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0x50,0x10,
		0x50,0x10,	0x90,0x08,	0xa7,0x08,	0xa8,0x88,	0xb0,0x70,	0x70,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0xf0,
		0x7f,0xf0,	0xff,0xf8,	0xff,0xf8,	0xf8,0xf8,	0xf0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 4
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0x50,0x10,
		0x50,0x30,	0x90,0x40,	0xa7,0x80,	0xa8,0x80,	0xb0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0xf0,
		0x7f,0xf0,	0xff,0xc0,	0xff,0x80,	0xf8,0x80,	0xf0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 5
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0xe8,	0x51,0x10,
		0x52,0x00,	0x92,0x00,	0xa6,0x00,	0xa8,0x00,	0xb0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0x10,
		0x7e,0x00,	0xfe,0x00,	0xfe,0x00,	0xf8,0x00,	0xf0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 6
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x60,	0xa0,0x40,	0xa0,0x80,	0xa1,0x00,	0x51,0x00,
		0x52,0x00,	0x92,0x00,	0xa6,0x00,	0xa8,0x00,	0xb0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xe0,	0xff,0xc0,	0xff,0x80,	0xff,0x00,	0x7f,0x00,
		0x7e,0x00,	0xfe,0x00,	0xfe,0x00,	0xf8,0x00,	0xf0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 7
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x60,	0xa0,0x40,	0xa0,0x80,	0xa1,0x00,	0x51,0x00,
		0x52,0x00,	0x32,0x00,	0x36,0x00,	0x08,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xe0,	0xff,0xc0,	0xff,0x80,	0xff,0x00,	0x7f,0x00,
		0x7e,0x00,	0x3e,0x00,	0x3e,0x00,	0x08,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,
	},
	{	// Phase 8
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x60,	0xa0,0x40,	0xa0,0x80,	0xa1,0x00,	0x52,0x00,
		0x7c,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xe0,	0xff,0xc0,	0xff,0x80,	0xff,0x00,	0x7e,0x00,
		0x7c,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 9
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0x00,	0x52,0x00,	0x52,0x00,	0xa2,0x00,	0xa1,0x00,	0xa1,0x00,	0x52,0x00,
		0x7c,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0x00,	0x7e,0x00,	0x7e,0x00,	0xfe,0x00,	0xff,0x00,	0xff,0x00,	0x7e,0x00,
		0x7c,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 10
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0x00,	0x52,0x00,	0x52,0x00,	0xa2,0x00,	0xad,0x00,	0xb3,0x00,	0x40,0x00,
		0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0x00,	0x7e,0x00,	0x7e,0x00,	0xfe,0x00,	0xff,0x00,	0xf3,0x00,	0x40,0x00,
		0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00
	}
#if 0
	{	// Phase 0
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
		0x08,0x01,	0x3c,0x01,	0x4c,0x01,	0x42,0x01,	0x30,0x01,	0x0c,0x01,	0x02,0x00,	0x01,0x00,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 1
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
		0x08,0x01,	0x3c,0x01,	0x4c,0x01,	0x42,0x01,	0x30,0x01,	0x0c,0x01,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 2
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
		0x08,0x01,	0x3c,0x01,	0x4c,0x01,	0x42,0x01,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 3
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
		0x08,0x01,	0x3c,0x01,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 4
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 5
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 6
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 7
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	}
#endif
};

extern const unsigned char gBusyLinkCursor[kNumCursorPhases][68] = {
	{	// Phase 0
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0x50,0x10,
		0x50,0x10,	0x90,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0xff,0xf8,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0xf0,
		0x7f,0xf0,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8
	},
	{	// Phase 1
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0x50,0x10,
		0x50,0x10,	0x90,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0xf0,	0xa0,0x80,	0xa1,0x00,	0xff,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0xf0,
		0x7f,0xf0,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf0,	0xff,0x80,	0xff,0x00,	0xff,0x00
	},
	{	// Phase 2
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0x50,0x10,
		0x50,0x10,	0x90,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0xf0,	0x70,0x80,	0x0d,0x00,	0x03,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0xf0,
		0x7f,0xf0,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf0,	0x7f,0x80,	0x0f,0x00,	0x03,0x00
	},
	{	// Phase 3
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0x50,0x10,
		0x50,0x10,	0x90,0x08,	0xa7,0x08,	0xa8,0x88,	0xb0,0x70,	0x70,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0xf0,
		0x7f,0xf0,	0xff,0xf8,	0xff,0xf8,	0xf8,0xf8,	0xf0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 4
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0x08,	0x50,0x10,
		0x50,0x30,	0x90,0x40,	0xa7,0x80,	0xa8,0x80,	0xb0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0xf0,
		0x7f,0xf0,	0xff,0xc0,	0xff,0x80,	0xf8,0x80,	0xf0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 5
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x08,	0xa0,0x08,	0xa0,0x08,	0xa0,0xe8,	0x51,0x10,
		0x52,0x00,	0x92,0x00,	0xa6,0x00,	0xa8,0x00,	0xb0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xf8,	0xff,0xf8,	0xff,0xf8,	0xff,0xf8,	0x7f,0x10,
		0x7e,0x00,	0xfe,0x00,	0xfe,0x00,	0xf8,0x00,	0xf0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 6
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x60,	0xa0,0x40,	0xa0,0x80,	0xa1,0x00,	0x51,0x00,
		0x52,0x00,	0x92,0x00,	0xa6,0x00,	0xa8,0x00,	0xb0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xe0,	0xff,0xc0,	0xff,0x80,	0xff,0x00,	0x7f,0x00,
		0x7e,0x00,	0xfe,0x00,	0xfe,0x00,	0xf8,0x00,	0xf0,0x00,	0x70,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 7
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x60,	0xa0,0x40,	0xa0,0x80,	0xa1,0x00,	0x51,0x00,
		0x52,0x00,	0x32,0x00,	0x36,0x00,	0x08,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xe0,	0xff,0xc0,	0xff,0x80,	0xff,0x00,	0x7f,0x00,
		0x7e,0x00,	0x3e,0x00,	0x3e,0x00,	0x08,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,
	},
	{	// Phase 8
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0xe0,	0x50,0x30,	0x50,0x60,	0xa0,0x40,	0xa0,0x80,	0xa1,0x00,	0x52,0x00,
		0x7c,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0xe0,	0x7f,0xf0,	0x7f,0xe0,	0xff,0xc0,	0xff,0x80,	0xff,0x00,	0x7e,0x00,
		0x7c,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 9
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0x00,	0x52,0x00,	0x52,0x00,	0xa2,0x00,	0xa1,0x00,	0xa1,0x00,	0x52,0x00,
		0x7c,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0x00,	0x7e,0x00,	0x7e,0x00,	0xfe,0x00,	0xff,0x00,	0xff,0x00,	0x7e,0x00,
		0x7c,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00
	},
	{	// Phase 10
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		1,	/*hot spot vertical			 */
		1,	/*hot spot horizontal			 */

		/* data */
		0x00,0x00,	0x3f,0x00,	0x52,0x00,	0x52,0x00,	0xa2,0x00,	0xad,0x00,	0xb3,0x00,	0x40,0x00,
		0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,
		
		/* mask */
		0x00,0x00,	0x3f,0x00,	0x7e,0x00,	0x7e,0x00,	0xfe,0x00,	0xff,0x00,	0xf3,0x00,	0x40,0x00,
		0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00,	0x00,0x00
	}
#if 0
	{	// Phase 0
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */
	
		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
		0x08,0x01,	0x3c,0x21,	0x4c,0x71,	0x42,0x71,	0x30,0xf9,	0x0c,0xf9,	0x02,0x00,	0x01,0x00,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 1
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */
	
		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
		0x08,0x01,	0x3c,0x21,	0x4c,0x71,	0x42,0x71,	0x30,0xf9,	0x0c,0xf9,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 2
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */
	
		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
		0x08,0x01,	0x3c,0x21,	0x4c,0x71,	0x42,0x71,	0x3f,0x06,	0x0f,0x06,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 3
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */
	
		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
		0x08,0x01,	0x3c,0x21,	0x7f,0x8f,	0x7f,0x8f,	0x3f,0x06,	0x0f,0x06,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 4
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */
	
		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x12,0x5c,	0x09,0x2a,
		0x0f,0xff,	0x3f,0xdf,	0x7f,0x8f,	0x7f,0x8f,	0x3f,0x06,	0x0f,0x06,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 5
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */
	
		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x24,0x00,	0x13,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xdf,	0x7f,0x8f,	0x7f,0x8f,	0x3f,0x06,	0x0f,0x06,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 6
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */
	
		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x24,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xdf,	0x7f,0x8f,	0x7f,0x8f,	0x3f,0x06,	0x0f,0x06,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	},
	{	// Phase 7
		16,	/*cursor size, valid values are 16 and 32*/
		1,	/*1 bit per pixel (only value right now) */
		2,	/*hot spot vertical			 */
		2,	/*hot spot horizontal			 */
	
		/* data */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xdf,	0x7f,0x8f,	0x7f,0x8f,	0x3f,0x06,	0x0f,0x06,	0x03,0xfe,	0x01,0xf8,
		
		/* mask */
		0x00,0x00,	0x00,0x00,	0x38,0x00,	0x3c,0x00,	0x3c,0x00,	0x1f,0xe0,	0x1f,0xfc,	0x0f,0xfe,
		0x0f,0xff,	0x3f,0xff,	0x7f,0xff,	0x7f,0xff,	0x3f,0xff,	0x0f,0xff,	0x03,0xfe,	0x01,0xf8
	}
#endif
};


int main()
{		
	NetPositive *app;

	app = new NetPositive();

	app->Run();

	app->WritePrefs();
	app->Cleanup();

	app->KillThreads();	
	
	delete(app);
	return(0);
}


// ===========================================================================

NetPositive::NetPositive() : BApplication(kApplicationSig)
{
	mLaunchRefs = false;
	mFilePanel = NULL;
	mIsQuitting = false;
}

NetPositive::~NetPositive()
{
	mIsQuitting = true;
	delete mFilePanel;
}

bool 
NetPositive::QuitRequested()
{
	mIsQuitting = true;
	return BApplication::QuitRequested();
}


void NetPositive::ReadyToRun()
{
	// Call the static version of initialization.  We can
	// also call this routine from replicant startup code.
	Init();

//	Default Startup Page	
	if (mLaunchRefs == false)
		if (!NewWindow()) {
			BString temp = kDefaultStartupURL;
			NewWindowFromURL(temp);
		}
}

static status_t	determine_image_file(entry_ref *ref)
{
	return be_roster->FindApp(kApplicationSig, ref);
}

void NetPositive::Init()
{
	// gRunningAsReplicant must be correctly set before
	// this is called.

	static bool initialized = false;
	
	if (initialized)
		return;
	initialized = true;
	
	char title[B_FILE_NAME_LENGTH + 1];
	app_info info;
	long err;

	if (gRunningAsReplicant) {
		entry_ref	ref;
		if (determine_image_file(&ref) == B_OK) {
			gImageRef = ref;
		}
	} else {
		err = be_app->GetAppInfo(&info);
		gImageRef = info.ref;
	}
	BEntry appFile(&gImageRef);
	
	appFile.GetName(title);
	pprint("Launching '%s'...",title);
	BFile file(&appFile, B_READ_ONLY);
	BAppFileInfo appFileInfo(&file);
	version_info versionInfo;
	if (!appFileInfo.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND)) {
		sShortVersionString = kApplicationName;
		sShortVersionString += " ";
		sLongVersionString = sShortVersionString;
		sShortVersionString += versionInfo.short_info;
		sLongVersionString += versionInfo.long_info;
		sVersionNumber = versionInfo.short_info;
	}
	
	{
		// Put this inside its own block to shorten the lifetime of the BScreen object.
		sMainScreenColorSpace = BScreen(B_MAIN_SCREEN_ID).ColorSpace();
	}
	
	gTranslatorRoster = new BTranslatorRoster;
	GifTranslator *lGifTranslator = new GifTranslator;
	gTranslatorRoster->AddTranslator(lGifTranslator);

	InitPrefs();
#ifdef ADFILTER
	AdFilter::Init();
#endif
	Protocol::Init();
	sGlobalBasicAuthFolder = BasicAuthFolder::CreateFolderWatcher();
	sGlobalFontSubFolder = FontSubFolder::CreateFolderWatcher();
	sGlobalHistoryFolder = HistoryFolder::CreateFolderWatcher();
	sGlobalBookmarkFolder = BookmarkFolder::CreateFolderWatcher();
	thread_id tid = spawn_thread(UResourceCache::Init, "Cache init", B_NORMAL_PRIORITY - 1, 0);
	resume_thread(tid);
	AddThread(tid);
	tid = spawn_thread(Cookie::Open, "Cookie init", B_LOW_PRIORITY + 2, 0);
	resume_thread(tid);
	AddThread(tid);
	Image::Open();			// Open Image cache
	srand((unsigned int)system_time() % 0xffffffff);
	if (!gConnectionMgr)
		gConnectionMgr = new ConnectionManager;
	HTMLView::Init();
	BuildMenuList();
	
	gWindowTitle = gPreferences.FindString("WindowTitle");
	
#ifdef JAVASCRIPT
	jseInitializeEngine();
#endif

#ifdef PLUGINS
	InitPlugins();
#endif
}

bool NetPositive::IsQuitting()
{
	return mIsQuitting;
}

void NetPositive::SetQuitting(bool mQuit)
{
	mIsQuitting = mQuit;
}

void NetPositive::Cleanup()
{
	delete sGlobalBasicAuthFolder;
	delete sGlobalFontSubFolder;
	delete sGlobalHistoryFolder;
	delete sGlobalBookmarkFolder;

#ifdef PLUGINS
	KillPlugins();
#endif
	Image::Close();
	Cookie::Close();
	UResourceCache::Close();
	Protocol::Cleanup();
	HTMLView::Cleanup();
	BeDrawPort::Cleanup();
#ifdef JAVASCRIPT
	jseTerminateEngine();
#endif
}

void NetPositive::KillThreads()
{
	// The app is about to quit, and we need to shut down
	// any threads that are still running hard.  This isn't
	// the time for subtlety.
	
	for (int i = mThreads.CountItems(); i >= 0 ; i--) {
		thread_id tid = (thread_id)mThreads.ItemAt(i);
		if (tid)
			kill_thread(tid);
		mThreads.RemoveItem(i);
	}
}


void
NetPositive::ArgvReceived(
	int32	argc,
	char	**argv)
{
	Init();
	for (int32 i = 1; i < argc; i++) {
		if (strcasecmp(argv[i], kEZDebugOption) == 0)
			gEZDebug = true;
		else if (strcasecmp(argv[i], kDumpDataOption) == 0)
			gDumpData = true;
		else if (strcasecmp(argv[i], kCleanDumpOption) == 0){
			gDumpData = true;
			gCleanDump = true;
		}
		else {
			BString url(argv[i]);
			if (strstr(argv[i], "://") == 0) {
				if (*argv[i] != '/') {
					char cwd[B_PATH_NAME_LENGTH];
					getcwd(cwd, B_PATH_NAME_LENGTH);
					url = (BString(cwd) += "/") += url;
					
				}
				url = BString("file://") += url;
				EscapeURL(url);
			}
			bool fullScreen = gPreferences.FindBool("FullScreen");
			if (NewWindowFromURL(url, N_USE_DEFAULT_CONVERSION, NULL, fullScreen) != NULL)
				mLaunchRefs = true;
		}
	}
}

void NetPositive::InitPrefs()
{
	gPreferences.MakeEmpty();
	const BoolPref *boolItem = GetDefaultBoolPrefs();
	while (boolItem->prefName && *(boolItem->prefName)) {
		gPreferences.AddBool(boolItem->prefName, boolItem->value);
		boolItem++;
	}

	const Int32Pref *int32Item = GetDefaultInt32Prefs();
	while (int32Item->prefName && *(int32Item->prefName)) {
		gPreferences.AddInt32(int32Item->prefName, int32Item->value);
		int32Item++;
	}

	const StringPref *stringItem = GetDefaultStringPrefs();
	while (stringItem->prefName && *(stringItem->prefName)) {
		gPreferences.AddString(stringItem->prefName, stringItem->value);
		stringItem++;
	}
	
	gPreferences.AddString("CacheDirectory", UResourceCache::GetCacheLocation());

	gPreferences.AddRect("DefaultBrowserWindowRect", 
		BRect(90, 32, 32 + NP_HTML_WIDTH+14, 14 + 32 + 90 + NP_HTML_HEIGHT+14));
	
	gPreferences.AddRect("DownloadWindowRect", BRect(0,0,0,0));
	
	BMessage tmp;
	gPreferences.AddMessage("PreviousDownloads", &tmp);

	const FontPrefItem *item = GetDefaultFontPrefs();
	while (item->encName && *(item->encName)) {
		BString name = item->encName;
		name += "ProFace";
		gPreferences.AddString(name.String(), item->proFace);
		
		name = item->encName;
		name += "ProSize";
		gPreferences.AddInt32(name.String(), item->proSize);
		
		name = item->encName;
		name += "ProMinSize";
		gPreferences.AddInt32(name.String(), item->proMinSize);
		
		name = item->encName;
		name += "FixFace";
		gPreferences.AddString(name.String(), item->fixFace);
		
		name = item->encName;
		name += "FixSize";
		gPreferences.AddInt32(name.String(), item->fixSize);
		
		name = item->encName;
		name += "FixMinSize";
		gPreferences.AddInt32(name.String(), item->fixMinSize);

		item++;
	}

	ReadPrefs();
}

void NetPositive::SetCursor(const void *cursor)
{
	static const void *prevCursor = NULL;
	if (cursor != prevCursor) {
		prevCursor = cursor;
		BApplication::SetCursor(cursor);
	}
}

color_space NetPositive::MainScreenColorSpace()
{
	// BScreen::ColorSpace() is a synchronous app_server call.  Cache it.

	// Ideally, we should maintain a separate color_space cache value for each window
	// and return the appropriate value for the window making the request, making sure
	// that we update the cached values if the bit depth for a window changes.  However,
	// this is a real pain, and most of the time, there will be only one bit depth that
	// never changes.
	return sMainScreenColorSpace;
}


// ===========================================================================

//	Return the size of the file in a nice formatted string

void ByteSizeStr(int size, BString& sizeStr)
{
	char str[1024];
	if (size >= 1024)
		if (size >= 1024*1024)
			sprintf(str,kByteSizeMBLabel,(float)size/(1024*1024));		// Meg
		else
			sprintf(str,kByteSizeKBLabel,(size + 512)/1024);					// Kilo
	else {
		if (size == 1)
			strcpy(str,kByteSizeOneByteLabel);
		else
			sprintf(str,kByteSizeManyBytesLabel,size);							// Bytes
	}
	sizeStr = str;
}

//	Position Window on screen

void PositionWindowRect(BRect *r)
{
	BScreen screen( B_MAIN_SCREEN_ID );
	BRect screenR = screen.Frame();
	
	BWindow *w;
	BRect oldR = *r;
	int i = 0;
	while ((bool)(w = be_app->WindowAt(i++))) {
		BRect wr = w->Frame();
		if (wr.top == r->top && wr.left == r->left) {
			r->OffsetBy(25,25);
			if (r->top > (screenR.bottom/3)*2) {	// More than 2/3 the way down
				*r = oldR;
				r->OffsetBy(50,0);
			}
			i = 0;
		}
	}
	
	if (r->right > screenR.right)
		r->right = screenR.right - 8;
	if (r->bottom > screenR.bottom)
		r->bottom = screenR.bottom - 8;
}

void CenterWindowRect(BRect *r)
{
	for (int i = 0; i < be_app->CountWindows(); i++) {
		BWindow *w = be_app->WindowAt(i++);
		if (w->IsFront()) {
			BRect wr = w->Frame();
			r->OffsetTo(wr.left + (wr.Width() - r->Width()) / 2, wr.top + (wr.Height() - r->Height()) / 2);
			return;
		}
	}
	BScreen screen( B_MAIN_SCREEN_ID );
	BRect screenR = screen.Frame();
	r->OffsetTo(screenR.left + (screenR.Width() - r->Width()) / 2, screenR.top + (screenR.Height() - r->Height()) / 2);
}

void NetPositive::FilePanelClosed(BMessage *)
{
	pprint("FilePanelClosed");
}

void NetPositive::MessageReceived(BMessage *msg)
{
	Init();
//	void* obj;
	switch (msg->what) {
//		Open a new file
			
#ifndef R4_COMPATIBLE
		case B_SILENT_RELAUNCH:
			NewWindow();
			break;
			
		case B_PRINTER_CHANGED:
			if (gPrintSettings) {
				delete gPrintSettings;
				gPrintSettings = NULL;
			}
			break;
#endif

		case OPEN_FILE: {
			pprint("run_open_panel");
//			run_open_panel();
			if (!sOpenFilePanel)
				sOpenFilePanel = new BFilePanel(B_OPEN_PANEL, 0, sFilePanelDir);
			sOpenFilePanel->Show();
			break;
		}

		case msg_WritePrefs:
			WritePrefs();
			break;

		case msg_ReloadAll:
			ReloadAllWindows();
			break;
			
		case FolderWatcher::kFileAdded:
		case FolderWatcher::kFileRemoved:
		case FolderWatcher::kFileChanged:
#ifdef ADFILTER
			AdFilter::FolderWatcherMsg(msg);
#endif
			if (sGlobalBasicAuthFolder)
				sGlobalBasicAuthFolder->MessageReceived(msg);
			if (sGlobalFontSubFolder)
				sGlobalFontSubFolder->MessageReceived(msg);
			if (sGlobalHistoryFolder)
				sGlobalHistoryFolder->MessageReceived(msg);
			if (sGlobalBookmarkFolder)
				sGlobalBookmarkFolder->MessageReceived(msg);
			break;

		case HTML_MSG + HM_ANCHOR:
		{
			BString url = msg->FindString("url");
			NewWindowFromURL(url);
			break; 
		}

		case DOWNLOADS_STOPPED:
		case DOWNLOADS_STARTED:
		case B_NETPOSITIVE_DOWN:
		case B_NETPOSITIVE_UP:
		{	
			int32 w = CountWindows();
			for (int32 i = 0; i < w;  i++) {
				BWindow *window = WindowAt(i);
				window->PostMessage(msg);
			}
			break;
		}
	
		case B_NETPOSITIVE_OPEN_URL:
		{
#ifdef PLUGINS
			bool returnStream;
			if (msg->FindBool("ReturnStream", &returnStream) == B_OK && returnStream) {
				thread_id tid = spawn_thread(HTMLView::CreateStreamIOThread, "Create BNetPositiveStreamIO", B_NORMAL_PRIORITY, DetachCurrentMessage());
				resume_thread(tid);
				NetPositive::AddThread(tid);
			} else {
#endif
				const char *url = msg->FindString("be:url");
				const char *postData = msg->FindString("PostData");
				if (!url)
					url = "";
				NewWindowFromURL(url, N_USE_DEFAULT_CONVERSION, NULL, false, true, true, NULL, true, kShowTitleUnspecified, postData);
				mLaunchRefs = true;
#ifdef PLUGINS
			}
#endif
		}
		break;
		
		case msg_WindowClosing:
		{	
			BMessage reply('npok');
			if (!gRunningAsReplicant) {
				bool dontClose = false;
				int32 w = CountWindows();
				void *theWindow;
				msg->FindPointer("window", &theWindow);
				bool fullScreen;
				status_t hasFullScreen = msg->FindBool("FullScreen", &fullScreen);
				for (int32 i = 0; i < w && !dontClose; i++) {
					BWindow *window = WindowAt(i);
					if ((window != theWindow) && dynamic_cast<HTMLWindow*>(window))
						dontClose = true;
					if(window != theWindow && window == DownloadManager::GetDownloadWindow() && !window->IsMinimized())
						dontClose = true;
				}
				if (!dontClose)
					dontClose = !(DownloadManager::RequestQuit());
				if (!dontClose) {
					if (gPreferences.FindBool("SaveWindowPos") && hasFullScreen == B_OK)
						gPreferences.ReplaceBool("FullScreen", fullScreen);
					PostMessage(B_QUIT_REQUESTED);
					mIsQuitting = true;
				}
				msg->SendReply(&reply);
				break;
			}
			msg->SendReply(&reply);
		}
			
		case msg_AddThread:
			if (!gRunningAsReplicant && msg->FindInt32("ThreadID"))
				mThreads.AddItem((void *)(msg->FindInt32("ThreadID")));
			break;
		
		case msg_RemoveThread:
			if (!gRunningAsReplicant && msg->FindInt32("ThreadID"))
				mThreads.RemoveItem((void *)(msg->FindInt32("ThreadID")));
			break;
			
		default:
			BApplication::MessageReceived(msg);
			break;
	}
}

void NetPositive::AddThread(thread_id tid)
{
	if (!gRunningAsReplicant) {
		BMessage msg(msg_AddThread);
		msg.AddInt32("ThreadID", (int32)tid);
		be_app->PostMessage(&msg);
	}
}

void NetPositive::RemoveThread(thread_id tid)
{
	if (!gRunningAsReplicant) {
		BMessage msg(msg_RemoveThread);
		msg.AddInt32("ThreadID", (int32)tid);
		be_app->PostMessage(&msg);
	}
}

//	All the simple alerts come through here

void SimpleAlert(const char* str)
{
#ifdef DEBUGMENU
	extern int32 numSpammers;
	if (numSpammers)
		return;
#endif
	if (gEZDebug)
		return;
	BString bstr(str);
	bstr.Truncate(256);
	BAlert *dang = new BAlert("",bstr.String(),kOKButtonTitle);
	dang->Go();
}


//	Display the about box

void NetPositive::AboutRequested()
{
	BRect rect(100,100,625,415);
	CenterWindowRect(&rect);
	NewWindowFromURL("netpositive:About.html", N_USE_DEFAULT_CONVERSION, NULL, false, false, false, &rect, false);
}

//	Turn a ref into a URL

bool NetPositive::RefToURL(entry_ref& ref, BString& url)
{
	bool result = false;
	
	BEntry entry(&ref, true);
	if (entry.IsFile()) {	
		// See if the ref is a bookmark.  If so, then get its URL
		// and open it up.
		BNode node(&ref);
		attr_info inf;
		char fileType[B_MIME_TYPE_LENGTH];
		BNodeInfo info(&node);
		if (info.GetType(fileType) == B_OK) {
			if (strcasecmp(fileType, kBookmarkMimeType) == 0 || strcasecmp(fileType, kDocBookmarkMimeType) == 0 ||
			    strcasecmp(fileType, "application/x-person") == 0) {
				if ((node.GetAttrInfo("META:url", &inf) == B_NO_ERROR && inf.type == B_STRING_TYPE && inf.size > 0) ||
					(node.GetAttrInfo("url", &inf) == B_NO_ERROR && inf.type == B_STRING_TYPE && inf.size > 0)) {
					char *str = (char *)malloc(inf.size + 1);
					if (node.ReadAttr("META:url", B_STRING_TYPE, 0, str, inf.size) == inf.size ||
						node.ReadAttr("url", B_STRING_TYPE, 0, str, inf.size) == inf.size) {
						str[inf.size] = 0;
						url = str;
						result = true;
					}
					free(str);			
				}
				return result;
			}
		}
		FileRefToURL(entry,url);
		return true;
	}
	pprint("RefToURL: Failed to identify ref");
	return result;
}

//	Somebody selected a file or a ref

void NetPositive::RefsReceived(BMessage *msg)
{
	ulong type;
	long count;

	Init();
	msg->GetInfo("refs",&type,&count);
	pprintBig("Got %d refs of type %d",count,type);
	for (long i = 0; i < count; i++) {
		entry_ref ref;
		if (msg->FindRef("refs",i,&ref) == B_NO_ERROR) {			
			BString url;
			if (RefToURL(ref,url)) {
				mLaunchRefs = true;
				bool fullScreen = gPreferences.FindBool("FullScreen");
				NewWindowFromURL(url, N_USE_DEFAULT_CONVERSION, NULL, fullScreen);
			}
		}
	}
}


// ===========================================================================
//	Create windows...


BWindow* NetPositive::NewWindow()
{
	const char *defaultURL = gPreferences.FindString("DefaultURL");
	bool fullScreen = gPreferences.FindBool("FullScreen");
	if (defaultURL && *defaultURL) {
		BString URL = defaultURL;
		return NewWindowFromURL(URL, N_USE_DEFAULT_CONVERSION, NULL, fullScreen);
	} else
		return NewWindowFromResource(NULL, false, N_USE_DEFAULT_CONVERSION, true, fullScreen);
}

//	Create a new window from a resource....
	
BWindow* NetPositive::NewWindowFromResource(
	UResourceImp	*r, 
	bool		openAsText,
	uint32		encoding,
	bool		showWindow,
	bool		fullScreen,
	bool		showToolbar,
	bool		showProgress,
	BRect		*rect,
	bool		resizable,
	int			showTitleInToolbar,
	BMessage 	*originatingMessage)
{
//	See what type of window to open?

	if (r) {
		const char *ct = r->GetContentType();
		if (ct) {			
			if ( (!strstr(ct, "image/jpeg")) && (!strstr(ct, "image/gif")) &&
				 (!strstr(ct, "text/text")) && (!strstr(ct, "text/html"))  &&
				 (!strstr(ct, "text/plain")) && (!strstr(ct, "image/jpg")) &&
				 (!strstr(ct, "image/png")) ) {
				//delete (r);
				r->RefCount(-1);
				(new BAlert(kErrorDialogTitle, kFileCannotBeOpenedError, kOKButtonTitle))->Go();
				return (NULL);
			}
		}
	}
	
	BRect frame;
	if (rect)
		frame = *rect;
	else {
		gPreferences.FindRect("DefaultBrowserWindowRect", &frame);
	//	BRect frame = GetDefaultHTMLWindowRect();
		PositionWindowRect(&frame);
		float i = frame.right - frame.left;
		if (i < 384)
			frame.left -= 384 - i;
		i = frame.bottom - frame.top;
		if (i < 128)
			frame.top -= 128 - i;
	}

	HTMLWindow	*window = new HTMLWindow(frame,r,openAsText, encoding, fullScreen, showToolbar, showProgress, resizable, showTitleInToolbar, originatingMessage);
	if (showWindow) {
		BAutolock lock(window);
		window->Show();
	}
	return window;
}
//	Create a new window from a url

BWindow* NetPositive::NewWindowFromURL(
	const BString	&url,
	uint32	encoding,
	const char *formData,
	bool fullScreen,
	bool showToolbar,
	bool showProgress,
	BRect *rect,
	bool resizable,
	int showTitleInToolbar,
	const char *postData,
	BMessage *originatingMessage)
{
	if (gPreferences.FindBool("DesktopMode") && !gRunningAsReplicant)
		for (int i = 0; i < be_app->CountWindows(); i++) {
			HTMLWindow *window = dynamic_cast<HTMLWindow*>(be_app->WindowAt(i));
			if (window && window->IsFullScreen()) {
				fullScreen = false;
				break;
			}
		}

	BString correctedURL = url;
	if (ValidateURL(correctedURL) == false && correctedURL.Length() != 0)
		return NULL;
		
	URLParser parser;
	parser.SetURL(correctedURL.String());
	if (parser.Scheme() == kFILE) {
		BString msg;
		UResourceImp *r;
		if (postData) {
			BString post(postData);
			r = GetUResourceFromForm(gConnectionMgr, correctedURL, &post, 0, msg);
		} else
			r = GetUResource(gConnectionMgr, correctedURL, 0, msg);
		return NewWindowFromResource(r, false, encoding, true, fullScreen, showToolbar, showProgress, rect, resizable, showTitleInToolbar, originatingMessage);
	}

/*
	if (!globalConnectionMgr)
		globalConnectionMgr = new ConnectionManager;

	BString msg;
	UResource *r = GetUResource(globalConnectionMgr, url,0, msg);	// Get the resource
	if (r == nil) {
		SimpleAlert(msg);
		return NULL;
	}
	return NewWindowFromResource(r, false, encoding);
*/
	HTMLWindow *window = dynamic_cast<HTMLWindow *>(NewWindowFromResource(NULL, false, encoding, false, fullScreen, showToolbar, showProgress, rect, resizable, showTitleInToolbar, originatingMessage));
	if (!window)
		return NULL;
	BMessage msg(HTML_MSG + HM_ANCHOR);
	msg.AddString("url",correctedURL.String());
	if (formData && *formData) {
		msg.AddBool("isForm", true);
		msg.AddString("post", formData);
	}
	window->PostMessage(&msg);
	{
		BAutolock lock(window);
		window->Show();
	}
	return window;
}

//	Create a new window with and error message

BWindow* NetPositive::NewWindowFromError(
	char	*heading, 
	char	*info,
	uint32	encoding)
{
	char str[1024];
	sprintf(str,"<HTML><TITLE>%s</TITLE></HEAD><BODY><H1>%s</H1><HR>%s<HR></BODY>",kApplicationName,heading,info);
	BString errorURL("http://error/");
	BString mimeType("text/html");
	UResourceImp *r = NewResourceFromData(str,strlen(str),errorURL,mimeType);
	return NewWindowFromResource(r, false, encoding);
}


void
NetPositive::ReadPrefs()
{
	BMessage tempPrefs(gPreferences);
	char pathname[B_PATH_NAME_LENGTH];
	GetPrefsFile(pathname);

	if (!(*pathname))
		return;
		
	BNode node(pathname);
	void *data = 0;
	
	if (node.InitCheck() == B_OK) do {
		char attrName[B_ATTR_NAME_LENGTH];
		if (node.GetNextAttrName(attrName) != B_OK)
			break;
		
		attr_info attrInfo;
		if (node.GetAttrInfo(attrName, &attrInfo) != B_OK)
			break;
			
		data = malloc(attrInfo.size);
		if (!data)
			break;
			
		if (node.ReadAttr(attrName, attrInfo.type, 0, data, attrInfo.size) != attrInfo.size){
			if(data)
				free(data);
			break;
		}
			
		tempPrefs.ReplaceData(attrName, attrInfo.type, data, attrInfo.size);
		if(data)
			free(data);
	} while (true);

	//this converts from old DownloadDirectory, if needed
	BString newDefaultDir = tempPrefs.FindString("DLDirectory");
	BString defaultDir = tempPrefs.FindString("DownloadDirectory");
	if(newDefaultDir.Length() == 0){
		if(defaultDir.Length() == 0){ //so there is not an old pref
			BPath settingsPath;
			find_directory(B_USER_DIRECTORY, &settingsPath);
			newDefaultDir = settingsPath.Path();
			newDefaultDir += kDefaultDownloadDirectory;
		} else {//grab the old pref then delete it
			newDefaultDir.SetTo(defaultDir);
			tempPrefs.RemoveName("DownloadDirectory");
		}
		tempPrefs.ReplaceString("DLDirectory", newDefaultDir.String());
	}
	//this converts from old CacheLocation, if needed
	newDefaultDir.SetTo(tempPrefs.FindString("CacheDirectory"));
	defaultDir.SetTo(tempPrefs.FindString("CacheLocation"));
	if(newDefaultDir.Length() == 0){
		if(defaultDir.Length() == 0){ //so there is not an old pref
			newDefaultDir.SetTo(UResourceCache::GetCacheLocation());
		} else { //grab the old pref and then delete it
			newDefaultDir.SetTo(defaultDir);
			tempPrefs.RemoveName("CacheLocation");
		}
		tempPrefs.ReplaceString("CacheDirectory", newDefaultDir.String());
	}

	//replace user directory specifics with __B_USER_DIRECTORY
	BPath userDirPath;
	find_directory(B_USER_DIRECTORY, &userDirPath);
	BString prefString(tempPrefs.FindString("CacheDirectory"));
	if(prefString.Length() > 0 && prefString.FindFirst("__B_USER_DIRECTORY") >= 0){
		prefString.ReplaceFirst("__B_USER_DIRECTORY", userDirPath.Path());
		tempPrefs.ReplaceString("CacheDirectory", prefString.String());
	}
	prefString.SetTo(tempPrefs.FindString("DLDirectory"));		
	if(prefString.Length() > 0 && prefString.FindFirst("__B_USER_DIRECTORY") >= 0){
		prefString.ReplaceFirst("__B_USER_DIRECTORY", userDirPath.Path());
		tempPrefs.ReplaceString("DLDirectory", prefString.String());
	}
	int32 version;
	if (tempPrefs.FindInt32("Version", &version) == B_OK && version == 2)
		gPreferences = tempPrefs;

	HTTP::SetProxyActive(gPreferences.FindBool("HTTPProxyActive"));
	FTP::SetProxyActive(gPreferences.FindBool("FTPProxyActive"));
	HTTP::SetProxyNameAndPort(gPreferences.FindString("HTTPProxyName"), gPreferences.FindInt32("HTTPProxyPort"),
		gPreferences.FindInt32("HTTPSProxyPort"));
	FTP::SetProxyNameAndPort(gPreferences.FindString("FTPProxyName"), gPreferences.FindInt32("FTPProxyPort"));

	UResourceCache::SetMaxCacheSize((uint64)gPreferences.FindInt32("MaxCacheSize") * 1024 * 1024);
	UResourceCache::SetCacheOption(gPreferences.FindInt32("CacheOption"));
	UResourceCache::SetCacheLocation(gPreferences.FindString("CacheDirectory"));

	int32 launchCount = gPreferences.FindInt32("LaunchCount");
	launchCount++;
	gPreferences.ReplaceInt32("LaunchCount", launchCount);

	if (!sFilePanelDir) {
		BEntry fileDefaultDir(gPreferences.FindString("DLDirectory"));
		sFilePanelDir = new entry_ref;
		fileDefaultDir.GetRef(sFilePanelDir);
	}
}


void
NetPositive::WritePrefs()
{
	gPreferences.ReplaceBool("HTTPProxyActive", HTTP::ProxyActive());
	gPreferences.ReplaceBool("FTPProxyActive", FTP::ProxyActive());
	BString name;
	int port, securePort;
	HTTP::GetProxyNameAndPort(name, &port, &securePort);
	gPreferences.ReplaceString("HTTPProxyName", name.String());
	gPreferences.ReplaceInt32("HTTPProxyPort", port);
	gPreferences.ReplaceInt32("HTTPSProxyPort", securePort);
	FTP::GetProxyNameAndPort(name, &port);
	gPreferences.ReplaceString("FTPProxyName", name.String());
	gPreferences.ReplaceInt32("FTPProxyPort", port);

	gPreferences.ReplaceInt32("MaxCacheSize", UResourceCache::GetMaxCacheSize() / 1024 / 1024);
	gPreferences.ReplaceInt32("CacheOption", UResourceCache::GetCacheOption());
	gPreferences.ReplaceString("CacheDirectory", UResourceCache::GetCacheLocation());
	

	char pathname[B_PATH_NAME_LENGTH];
	GetPrefsFile(pathname);
	if (!(*pathname))
		return;

	BNode node(pathname);
	if (node.InitCheck()) {
		BFile file(pathname, B_WRITE_ONLY | B_CREATE_FILE);
		node.SetTo(pathname);
		if (node.InitCheck())
			return;
	}

	//Since we want the settings directories to be portable across user accounts
	//(whenever we get those) we should convert the user directory into
	//the string "B_USER_DIRECTORY" (as we wrote it)
	BPath userDirPath;
	find_directory(B_USER_DIRECTORY, &userDirPath);
	BString prefString(gPreferences.FindString("CacheDirectory"));
	if(prefString.Length() > 0 && prefString.FindFirst(userDirPath.Path()) >= 0){
		prefString.ReplaceFirst(userDirPath.Path(), "__B_USER_DIRECTORY");
		gPreferences.ReplaceString("CacheDirectory", prefString.String());
	}
	prefString.SetTo(gPreferences.FindString("DLDirectory"));
	if(prefString.Length() > 0 && prefString.FindFirst(userDirPath.Path()) >= 0){
		prefString.ReplaceFirst(userDirPath.Path(), "__B_USER_DIRECTORY");
		gPreferences.ReplaceString("DLDirectory", prefString.String());
	}
	
	//these are deprecated, so we don't write them
	gPreferences.RemoveName("DownloadDirectory"); //replaced with DLDirectory
	gPreferences.RemoveName("CacheLocation"); //replace with CacheDirectory

	for (int i = 0; i < gPreferences.CountNames(B_ANY_TYPE); i++) {
		const char *name;
		type_code type;
		if (gPreferences.GetInfo(B_ANY_TYPE, i, &name, &type))
			break;
		const void *data;
		ssize_t size;
		if (gPreferences.FindData(name, type, &data, &size))
			break;
		if (node.WriteAttr(name, type, 0, data, size) != size)
			break;
	}

	//put user visible prefs back so that OK from the prefs window doesn't
	//put __B_USER_DIRECTORY in use
	prefString.SetTo(gPreferences.FindString("CacheDirectory"));
	if(prefString.Length() > 0 && prefString.FindFirst("__B_USER_DIRECTORY") >= 0){
		prefString.ReplaceFirst("__B_USER_DIRECTORY", userDirPath.Path());
		gPreferences.ReplaceString("CacheDirectory", prefString.String());
	}
	prefString.SetTo(gPreferences.FindString("DLDirectory"));		
	if(prefString.Length() > 0 && prefString.FindFirst("__B_USER_DIRECTORY") >= 0){
		prefString.ReplaceFirst("__B_USER_DIRECTORY", userDirPath.Path());
		gPreferences.ReplaceString("DLDirectory", prefString.String());
	}
}


void
NetPositive::GetPrefsFile(char *pathname)
{
	*pathname = 0;
	BPath thePath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &thePath, true) != B_NO_ERROR)
		return;

	if (thePath.Append(kApplicationName) != B_NO_ERROR)
		return;

	mkdir(thePath.Path(), 0777);

	if (thePath.Append(kSettingsFolderName) != B_NO_ERROR)
		return;

	strcpy(pathname, thePath.Path());
}


void
NetPositive::ReloadAllWindows()
{
	int32 w = CountWindows();
	for (int32 i = 0; i < w; i++) {
		BWindow *window = WindowAt(i);
		if (window)
			window->PostMessage(msg_RelayoutView, window);		
	}
}

void NetPositive::HandleCopyTarget(BMessage *msg)
{
	entry_ref directory;
	const char *name;
	const char *url;
	BMessage data;

	if (msg->FindString("name", &name) == B_OK 
	    && msg->FindRef("directory", &directory) == B_OK
		&& msg->FindMessage("be:originator-data", &data) == B_OK
		&& data.FindString("be:url", &url) == B_OK) {
	
		const char *replyType;
		if (msg->FindString("be:filetypes", &replyType) != B_OK)
			// drag recipient didn't ask for any specific type,
			// create a bookmark file as default
			replyType = "application/x-vnd.Be-bookmark";
		
		BDirectory dir(&directory);	
		BFile file(&dir, name, B_READ_WRITE);
		if (file.InitCheck() == B_OK) {					
			if (strcmp(replyType, "application/x-vnd.Be-bookmark") == 0)
				// we got a request to create a bookmark, stuff it with
				// the url attribute
				file.WriteAttr("META:url", B_STRING_TYPE, 0, url, strlen(url) + 1);
			
			else if (strcasecmp(replyType, "text/plain") == 0)
				// create a plain text file, stuff it with the url as text
				file.Write(url, strlen(url));

			BNodeInfo fileInfo(&file);
			fileInfo.SetType(replyType);
		}
	}
}


void NetPositive::DragLink(const char *url, const BRect rect, BView *view, BBitmap *dragImage, BPoint offset)
{
	BMessage dragMessage(B_SIMPLE_DATA);
	dragMessage.AddInt32("be:actions", B_COPY_TARGET);
	dragMessage.AddString("be:types", B_FILE_MIME_TYPE);
	dragMessage.AddString("be:filetypes", "application/x-vnd.Be-bookmark");
	dragMessage.AddString("be:filetypes", "text/plain");
	dragMessage.AddString("be:clip_name", "Bookmark");
	dragMessage.AddString("be:url", url);

	BString cookie;
	URLParser urlParser;
	urlParser.SetURL(url);
	Cookie::Add(urlParser, cookie);
	if(cookie.Length() > 0)
		dragMessage.AddString("be:cookie", cookie.String());

	BMessage data;
	data.AddString("be:url", url);
	dragMessage.AddMessage("be:originator-data", &data);
	if (dragImage)
		view->DragMessage(&dragMessage, new BBitmap(dragImage), B_OP_OVER, offset, view);
	else
		view->DragMessage(&dragMessage, rect, view);
}


const char* GetShortVersionString()
{
	return sShortVersionString.String();
}

const char* GetVersionNumber()
{
	return sVersionNumber.String();
}


const char*	GetLongVersionString()
{
	return sLongVersionString.String();
}

status_t GetIcon(BBitmap *icon)
{
	BEntry entry(&gImageRef);
	BFile file(&entry, B_READ_ONLY);
	BAppFileInfo appFileInfo(&file);
	return appFileInfo.GetIcon(icon, B_LARGE_ICON);
}




// ===========================================================================
//	Get a required resource from the app, a resource file or the resFolder

void *GetNamedResourceFromFile(BResources& resources,const char *name, size_t& size)
{
	long idFound;
	char *data = NULL;
	size = 0;
	
	if (resources.GetResourceInfo('data',name,&idFound,&size)) {
		data = (char *)malloc(size);
//		NP_ASSERT(data);
		resources.ReadResource('data',idFound,data,0,size);
	}
	pprint("GetResourceInfo: %s: %d bytes",name,size);
	return data;
}



void *GetNamedResource(const char *name, size_t& size)
{
	app_info info;
	char *data;
	long err;
	
	pprint("GetNamedResource: %s",name);

	BEntry	entry;
	err = entry.SetTo(&gImageRef);

	BFile file(&entry,O_RDONLY);		// Application file
	BResources resources(&file);
	
	data = (char *)GetNamedResourceFromFile(resources,name,size);
	if (data) {
		pprint("Got '%s' from app",name);
		return data;						// Resource was in application
	}
		
	
	pprint("Failed to read '%s'",name);
	return NULL;
}

static BMidiSynthFile* song;
static BMidiSynth* liveSynth;
static BMidiPort* livePort;
static TLocker midiLocker("MIDI Locker");

class SongData {
public:
	SongData(entry_ref *ref, bool loop) : mRef(*ref), mLoop(loop) {}
	entry_ref	mRef;
	bool		mLoop;
};

void NetPositive::InitMidi()
{
	static bool MidiInitialized = false;
	if (MidiInitialized)
		return;
	MidiInitialized = true;
	
	  new BSynth();
	  status_t err = be_synth->LoadSynthData(B_BIG_SYNTH);
	  if (err < B_NO_ERROR) {
		err = be_synth->LoadSynthData(B_LITTLE_SYNTH);
		if (err < B_NO_ERROR) {
		  delete be_synth;
		  be_synth = NULL;
		  return;
		}
	  }
	  be_synth->SetVoiceLimits(32, 0, 7);
}

int32 StopSongEntry(void *)
{
	midiLocker.Lock();
	if (song) {
		song->Fade();
		song->Stop();
	}
	delete song;
	delete liveSynth;
	delete livePort;
	song = NULL;
	liveSynth = NULL;
	livePort = NULL;

	midiLocker.Unlock();
	NetPositive::RemoveThread(find_thread(NULL));
	return 0;
}


int32 StartSongEntry(void *args)
{
	SongData *data = (SongData *)args;
	midiLocker.Lock();
	if (!be_synth)
		NetPositive::InitMidi();
	if (!be_synth) {
		midiLocker.Unlock();
		delete data;
		return 0;
	}

	song = new BMidiSynthFile();
		liveSynth = new BMidiSynth();
	livePort = new BMidiPort();
	if (livePort)
		livePort->Connect(liveSynth);

	if (song) {
//		StopSongEntry(NULL);
		if (song->LoadFile(&data->mRef) == B_NO_ERROR)
			song->Start();
		song->EnableLooping(data->mLoop);
	}
	midiLocker.Unlock();
	delete data;
	NetPositive::RemoveThread(find_thread(NULL));
	return 0;
}

void NetPositive::StartSong(entry_ref *ref, bool loop)
{
	SongData *data = new SongData(ref, loop);
	thread_id tid = spawn_thread(StartSongEntry, "Start MIDI", B_NORMAL_PRIORITY - 1, data);
	resume_thread(tid);
	NetPositive::AddThread(tid);
}

void NetPositive::StopSong()
{
	thread_id tid = spawn_thread(StopSongEntry, "Stop MIDI", B_NORMAL_PRIORITY - 1, NULL);
	resume_thread(tid);
	NetPositive::AddThread(tid);
}

void FFMSetMouseOverWindow(BWindow *window, BPoint offset)
{
	if (!focus_follows_mouse())
		return;
	
	offset.x += window->Frame().left;
	offset.y += window->Frame().top;
	set_mouse_position((int32)offset.x, (int32)offset.y);
}

void FFMWarpingWindow::Activate(bool on)
{
	BWindow::Activate(on);
	if (on)
		FFMSetMouseOverWindow(this, BPoint(40, 30));
}

void FFMWarpingWindow::Show()
{
	BWindow::Show();
	FFMSetMouseOverWindow(this, BPoint(40, 30));
}


