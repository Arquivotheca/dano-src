/*************************************************************************
/
/	KeyboardModule.h
/
/	Written by Robert Polic
/
/	Modified on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#ifndef _KEYBOARD_MODULE_H_
#define _KEYBOARD_MODULE_H_

#include <SupportDefs.h>

class	MinitelView;
class	Protocole;

enum	Modes
{
	eTeletel = 0,
	eTeletelEtendu,
	eTeletelCo,
	eMixteFr,
	eMixteUS,
	eTeleInfoFr,
	eTeleInfoUS
};


//========================================================================

class KeyboardModule
{
	public:
							KeyboardModule		(Protocole*,
												 MinitelView*);
		void				SetMode				(int16);
		void				Annulation			();
		void				Envoi				();
		void				Retour				();
		void				Suite				();
		void				Sommaire			();
		void				Repetition			();
		void				Connexion			();
		void				Correction			();
		void				Guide				();
		void				FctFromKeyboard		(int32);
		int32				FilterFctNumber		(int32 flag,
												 int32 code);
		void				EventFromKeyboard	(const char* ascii,
												 int32 modifiers,
												 int32 key);
		void				StdCharToFR			(uint16 c);
		void				StdCharToUS			(uint16 c);
		void				StdCharToVideotex	(uint16 c);

	private:
		int16				fMode;
		int16				fInsert;
		int32				fCompatX3;
		int32				fInvmajmin;
		MinitelView*		fAffiChageEcran;
		Protocole*			fProtocole;
};

#endif	/* _KEYBOARD_MODULE_H_ */
