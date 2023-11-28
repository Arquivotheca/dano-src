/*************************************************************************
/
/	Protocole.h
/
/	Modified by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#ifndef _PROTOCOLE_H_
#define _PROTOCOLE_H_

#include <Entry.h>
#include <File.h>
#include <stdlib.h>

class	Iso6429ScreenModule;
class	KeyboardModule;
class	MinitelView;
class	ModemModule;
class	StdScreenModule;
class	VirtScreenModule;


#define kENVOI			1
#define kRETOUR			2
#define kREPETITION		3
#define kGUIDE			4
#define kANNULATION		5
#define kSOMMAIRE		6
#define kCORRECTION		7
#define kSUITE			8


//========================================================================

class Protocole
{
	public:
							Protocole			(MinitelView*);
							~Protocole			();
		void				DoOpen				(entry_ref*);
		void				Reset				();
		void				ToggleCompatX3		(int32 flag);
		void				StdCharFromKbd		(unsigned char c);
		void				SpeCharFromKbd		(unsigned char c);
		void				CsiCharFromKbd		(unsigned char c);
		void				TeletelFctFromKbd	(unsigned char c);
		void				InfoFctFromKbd		(unsigned char c);
		void				CharFromModem		(unsigned char c);
		void				ConnexionFin		();
		void				StatusConnect		(int32 stat);
		void				Mode40Col			();
		void				Mode80Col			();
		void				RetourFrom80Col		();
		void				Goto80Col			();
		void				JeuAmericain		();
		void				JeuFrancais			();
		void				EventFromKeyboard	(const char* ascii,
												 int32 modifiers,
												 int32 key);

		KeyboardModule*		fClavier;
		ModemModule*		fModem;

	protected:
		void				Pro1				(unsigned char c);
		void				Pro2				(unsigned char c1,
												 unsigned char c2);
		void				Pro3				(unsigned char c1,
												 unsigned char c2,
												 unsigned char c3);
		void				RepStatusClavier	();
		void				RepStatusPro2		();

		bool				fClavierOK;
		bool				fModemOK;
		int16				fCompatX3;
		int16				fModeClavier;
		int16				fModeEcran;
		int32				fConnect;
		int32				fModeProt;
		int32				fTranspCount;
		unsigned char		fArg1;
		unsigned char		fArg2;
		unsigned char		fRlxFlag;
		Iso6429ScreenModule*fTraitEcranIso6429;
		MinitelView*		fAffiChageEcran;
		StdScreenModule*	fTraitEcran;
		VirtScreenModule*	fScrProc;
};
#endif
