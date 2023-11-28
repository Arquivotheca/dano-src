#ifndef FAT_BIT_DEFS_H
#define FAT_BIT_DEFS_H

#include <GraphicsDefs.h>

const int32 kIconEditorWidth = 445;
const int32 kIconEditorHeight = 435;

const char* const kLargeIconMimeType = "icon/large";
const char* const kMiniIconMimeType = "icon/mini";
const char* const kBitmapMimeType = "image/x-vnd.Be-bitmap";

const char* const kIconEditorPrefsfileName = "IconOMatic_settings";

const int32 msg_show_prefs = 'pref';

const rgb_color kViewGray = { 216, 216, 216, 255};
const rgb_color kWhite = { 255, 255, 255, 255};
const rgb_color kThinGray = { 245, 245, 245, 255};
const rgb_color kBlack = { 0, 0, 0, 255};
const rgb_color kDarkGray = { 160, 160, 160, 255};
const rgb_color kMediumGray = { 190, 190, 190, 255};
const rgb_color kGridGray = { 200, 200, 200, 255};
extern const rgb_color kLightGray;

#endif
