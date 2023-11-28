// ===========================================================================
//	Utils.h
// ===========================================================================
//	Copyright 1995 - 1996 Metrowerks Corporation. All rights reserved.

#ifndef _UTILS_H
#define _UTILS_H

#include <StorageDefs.h>
#include <Font.h>
#include <View.h>

struct TargetRec;
class BEntry;
class BMenu;
class BResources;
class BMenuItem;
class BTextView;

typedef int (*ListCompareFunc)(const void*, const void*);


	void 						PrintBytes(
									void*	inBytes,
									int32	inLength);

	char *						GetFileName(
									BEntry *		file);
	BRect						GetTextRect(
									const BRect & area);

	void						DeleteArgv(
									char**	inArgv);
	void						DeleteArgv(
									BList&	inArgv);

	void						CenterWindow(
									BWindow* 	inWindow);
	void						ValidateWindowFrame(
									BWindow& 	inWindow);
	void						ValidateWindowFrame(
									BRect 		inFrame,
									BWindow& 	inWindow);
	void						GetBFontFamilyAndStyle(
									const BFont&	inFont,
									font_family		outFamily,
									font_style		outStyle);


	void						FillFontSizeMenu(BMenu* sizeMenu);
	void						FillFontFamilyMenu(BMenu* fontMenu);
	void						MarkCurrentFontSize(BMenu* sizeMenu, float size);
	void						MarkCurrentFont(BMenu* fontMenu, 
												const font_family family,
												const font_style style,
												int32 firstFontItem = 0);

	bool						WindowIsHidden(
									BWindow*	inWindow);
	void						ShowAndActivate(
									BWindow*	inWindow);

	BBitmap*					LoadBitmap(
									const char * 	inBitMapData,
									int32			inWidth,
									int32			inHeight);
	BBitmap*					LoadBitmap(
									BResources&		inFile,
									type_code		inResType,
									int32			inResID,
									int32			inWidth,
									int32			inHeight);

	uint32 						MimeType(
									const char *	inType);
	uint32 						MimeType(
									const BEntry&	inEntry);
	bool						FixFileType(
									const BEntry* 		inEntry);
	bool						FixFileType(
									const BEntry*	inFile, 
									TargetRec*		inTargetArray,
									int32			inTargetCount);

	void						EnableItem(
									BMenu* 		inMenu, 
									uint32 		inCommand, 
									bool 		inEnabled);

	void						SetCommand(
									BMenuItem& 	inMenuItem, 
									uint32 		inCommand);
	void						SetItemMessageAndName(
									const BMenu*	inMenu,
									int32			inIndex,
									uint32			inCommand,
									const char *	inName);
	void						SwapRectToHost(
									BRect&	inoutRect);
	void						SwapRectToBig(
									BRect&	inoutRect);
	void						SwapRectLittleToHost(
									BRect&	inoutRect);

	void						DisallowInvalidChars(
									BTextView& 		inText);
	void						DisallowNonDigitChars(
									BTextView& 		inText);
	int32						GetValue(
									BTextView*	inView);

	int32						GlyphWidth(
									const char *	inBytes);

	void 						PrintColor(
									rgb_color color);

	int							ListCompareStrings(
									char ** inOne,
									char ** inTwo);


	inline void					PulseOn(
									BView*	inView)
								{
									inView->SetFlags(inView->Flags() | B_PULSE_NEEDED);	// turn on pulse
								}
	inline void					PulseOff(
									BView*	inView)
								{
									inView->SetFlags(inView->Flags() & ~B_PULSE_NEEDED);// turn off pulse
								}


//	Gray value to use for light shade
const uchar LTVAL = 235;
//	Gray value to use for dark shade
const uchar DKVAL = 190;


const rgb_color kLtGray = { LTVAL, LTVAL, LTVAL, 255 };
const rgb_color kDkGray = { DKVAL, DKVAL, DKVAL, 255 };
const rgb_color kFocusBoxGray = { 144, 144, 144, 255 };
const rgb_color kTransparent = { 255, 255, 255, 255 };	// really white

const rgb_color kMenuBodyGray = { 216, 216, 216, 255 };
const rgb_color kMenuLiteHilite = { 176, 176, 176, 255 };
const rgb_color kMenuDarkHilite = { 144, 144, 144, 255 };

const rgb_color kGrey120 = { 120, 120, 120, 255 };	// 15
const rgb_color kGrey136 = { 136, 136, 136, 255 };	// 17
const rgb_color kGrey144 = { 144, 144, 144, 255 };	// 18
const rgb_color kGrey152 = { 152, 152, 152, 255 };	// 19
const rgb_color kGrey176 = { 176, 176, 176, 255 };	// 22
const rgb_color kGrey208 = { 208, 208, 208, 255 };	// 26
const rgb_color kGrey216 = { 216, 216, 216, 255 };	// 27	default hilite color
// somewhere the system pallette changed 
const rgb_color kGrey217 = { 217, 217, 217, 255 };	// 27	in PR2


const rgb_color white	= { 255, 255, 255, 255 };
const rgb_color ltGray	= { 238, 238, 238, 255 };
const rgb_color mdGray	= { 210, 210, 210, 255 };
const rgb_color dkGray	= { 153, 153, 153, 255 };
const rgb_color black	= { 0, 0, 0, 255 };
const rgb_color red		= { 255, 0, 0, 255 };
const rgb_color green	= { 100, 238, 100, 255 };
const rgb_color blue	= { 0, 0, 255, 255 };
const rgb_color purple	= { 238, 238, 100, 255 };
const rgb_color pink	= { 255, 200, 200, 255 };
const rgb_color yellow	= { 238, 238, 100, 255 };
const rgb_color brown	= { 150, 150, 50, 255 };

	void						SetGrey(
									BView* 		inView,
									rgb_color	inColor);

enum SuffixType
{
	kInvalidSuffix,
	kHSuffix,			// any header suffix
	kCSuffix,			// any C file suffix
	kCPSuffix,			// any C++ file suffix
	kPCHSuffix,			// pch file suffix
	kDumpSuffix,		// disassembly suffix
	kObjectSuffix,		// library files
	kProjectSuffix,		// project files
	kEXPSuffix,			// exp files
	kXCOFFSuffix,		// XCOFF files
	kJavaSuffix,		// java source files
	kClassFileSuffix,	// java class files
	kZipFileSuffix		// java zip files
};

	SuffixType					GetSuffixType(
									const char *	inFileName);

#endif
