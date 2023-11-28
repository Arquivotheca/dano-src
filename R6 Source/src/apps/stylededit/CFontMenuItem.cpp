// ============================================================
//  CFontMenuItem.cpp	©1996 Hiroshi Lockheimer
// ============================================================

#include "CFontMenuItem.h"


CFontMenuItem::CFontMenuItem(
	const char	*label,
	BMessage 	*message,
	char 		shortcut,
	ulong 		modifiers)
		: BMenuItem(label, message, shortcut, modifiers)
{
}


void
CFontMenuItem::GetContentSize(
	float	*width,
	float	*height)
{
/*	BMenu		*menu = Menu();
	const char	*name = Label();
	
	font_info saveFontInfo;
	menu->GetFontInfo(&saveFontInfo);
	
	menu->SetFontName(name);	
	font_info theFontInfo;
	menu->GetFontInfo(&theFontInfo);
	
	*width = menu->StringWidth(name);
	*height = theFontInfo.ascent + theFontInfo.descent;
	
	menu->SetFontName(saveFontInfo.name);
*/
	BMenuItem::GetContentSize(width, height);
}


void
CFontMenuItem::DrawContent()
{
/*
	BMenu *menu = Menu();
	
	font_info saveFontInfo;
	menu->GetFontInfo(&saveFontInfo);
	
	menu->SetFontName(Label());	
	
	BMenuItem::DrawContent();
	
	menu->SetFontName(saveFontInfo.name);
*/
	BMenuItem::DrawContent();
}
