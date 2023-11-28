/*************************************************************************
/
/	KeyboardModule.cpp
/
/	Modified by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#include <string.h>
#include <Beep.h>

#include "KeyboardModule.h"
#include "MinitelView.h"
#include "Protocole.h"


#define kKBD_ESC			0x01
#define kKBD_BS				0x1e
#define kKBD_TAB			0x26
#define kKBD_DELETE			0x34
#define kKBD_ENTER			0x47
#define kKBD_ARROWUP		0x57
#define kKBD_PADENTER		0x5b
#define kKBD_ARROWLEFT		0x61
#define kKBD_ARROWDOWN		0x62
#define kKBD_ARROWRIGHT		0x63


//========================================================================

KeyboardModule::KeyboardModule(Protocole* protocole, MinitelView* view)
	: fMode				(eTeletel),
	  fInsert			(0),
	  fCompatX3			(0),
	  fInvmajmin		(1),
	  fAffiChageEcran	(view),
	  fProtocole		(protocole)
{
	//InitButtons();
}

//------------------------------------------------------------------------

void KeyboardModule::SetMode(int16 m)
{
	fMode = m;
	fInsert = 0;
}

//------------------------------------------------------------------------

void KeyboardModule::Annulation()	/* Cancel */
{
	if ((fMode == eTeleInfoFr) || (fMode == eTeleInfoUS))
		fProtocole->InfoFctFromKbd(0x51);
	else
		fProtocole->TeletelFctFromKbd((unsigned char)kANNULATION);
}

//------------------------------------------------------------------------

void KeyboardModule::Envoi()		/* Enter */
{
	if ((fMode == eTeleInfoFr) || (fMode == eTeleInfoUS))
		fProtocole->InfoFctFromKbd(0x4d);
	else
		fProtocole->TeletelFctFromKbd((unsigned char)kENVOI);
}

//------------------------------------------------------------------------

void KeyboardModule::Retour()		/* Back */
{
	if ((fMode == eTeleInfoFr) || (fMode == eTeleInfoUS))
		fProtocole->InfoFctFromKbd(0x52);
	else
		fProtocole->TeletelFctFromKbd((unsigned char)kRETOUR);
}

//------------------------------------------------------------------------

void KeyboardModule::Suite()		/* Next */
{
	if ((fMode == eTeleInfoFr) || (fMode == eTeleInfoUS))
		fProtocole->InfoFctFromKbd(0x6e);
	else
		fProtocole->TeletelFctFromKbd((unsigned char)kSUITE);
}

//------------------------------------------------------------------------

void KeyboardModule::Sommaire()		/* Summary */
{
	if ((fMode == eTeleInfoFr) || (fMode == eTeleInfoUS))
		fProtocole->InfoFctFromKbd(0x50);
	else
		fProtocole->TeletelFctFromKbd((unsigned char)kSOMMAIRE);
}

//------------------------------------------------------------------------

void KeyboardModule::Repetition()	/* Repeat */
{
	if ((fMode == eTeleInfoFr) || (fMode == eTeleInfoUS))
		fProtocole->InfoFctFromKbd(0x53);
	else
		fProtocole->TeletelFctFromKbd((unsigned char)kREPETITION);
}

//------------------------------------------------------------------------

void KeyboardModule::Connexion()	/* Conn/End */
{
	if ((fMode == eTeleInfoFr) || (fMode == eTeleInfoUS))
		fProtocole->ToggleCompatX3(1);
	fProtocole->ConnexionFin();
	fProtocole->ToggleCompatX3(fCompatX3);
}

//------------------------------------------------------------------------

void KeyboardModule::Correction()	/* Correction */
{
	if ((fMode == eTeleInfoFr) || (fMode == eTeleInfoUS))
		fProtocole->InfoFctFromKbd(0x6c);
	else
		fProtocole->TeletelFctFromKbd((unsigned char)kCORRECTION);
}

//------------------------------------------------------------------------

void KeyboardModule::Guide()		/* Guide */
{
	if ((fMode == eTeleInfoFr) || (fMode == eTeleInfoUS))
		fProtocole->InfoFctFromKbd(0x6d);
	else
		fProtocole->TeletelFctFromKbd((unsigned char)kGUIDE);
}

//------------------------------------------------------------------------

static void inv_maj_min(uint16 *c)
{
	if ((*c >= 'a') && (*c <= 'z')) 
	{
		*c = *c - 'a' + 'A';
		return;
	}
	if ((*c >= 'A') && (*c <= 'Z')) 
	{
		*c = *c - 'A' + 'a';
		return;
	}
	return;
}

//------------------------------------------------------------------------

void KeyboardModule::FctFromKeyboard(int32 num)
{
	switch (num)
	{
		case 0:
			Sommaire();
			break;

		case 1:
			Annulation();
			break;

		case 2:
			Retour();
			break;

		case 3:
			Repetition();
			break;

		case 4:
			Envoi();
			break;

		case 5:
			Correction();
			break;

		case 6:
			Guide();
			break;

		case 7:
			Suite();
			break;

		case 8:
			Connexion();
			break;

		default:
			break;
	}
}

//------------------------------------------------------------------------

int32 KeyboardModule::FilterFctNumber(int32 flag, int32 code)
{
	int32	fct = -1;

	switch (code)
	{
		case kKBD_ESC:
			fct = 8; // Cnx/Fin
			break;

		case kKBD_ARROWLEFT:
			fct = 0; // Sommaire
			break;

		case kKBD_ARROWRIGHT:
			fct = 6; // Guide
			break;

		case kKBD_ARROWUP:
			fct = 2; // Retour
			break;

		case kKBD_ARROWDOWN:
			fct = 7; // Suite
			break;

		case kKBD_ENTER:
			fct = 4; // Envoi
			break;

		case kKBD_TAB:
			(flag & B_SHIFT_KEY) ?
				fct = 2:
				fct = 7;
			break;

		case kKBD_BS:
			fct = 5; // Correction
			break;

		case kKBD_DELETE:
			fct = 1; // Annulation
			break;

		default:
			break;
	}
	return fct;
}

//------------------------------------------------------------------------

void KeyboardModule::EventFromKeyboard(const char* ascii, int32 modifiers, int32 key)
{
	int32	fct;

	switch (fMode)
	{
		case eTeletel:
			fct = FilterFctNumber(modifiers, key);
			if (fct != -1)
				FctFromKeyboard(fct);
			else
			{
				switch (key)
				{
					case kKBD_ARROWLEFT:
						if (modifiers & B_CONTROL_KEY)
							fProtocole->StdCharFromKbd(0x7f);
						break;

					case kKBD_ARROWDOWN: /* fleches non supportees */
					case kKBD_ARROWRIGHT:
					case kKBD_ARROWUP:	
					case kKBD_PADENTER: /* enter non supporte */
						beep();
						break;

					default :
						if (ascii[0] >= 0x20)
							StdCharToVideotex(ascii[0]);
						break;
				}
			}
			break;

		case eTeletelCo:
			fct = FilterFctNumber(modifiers, key);
			if (fct != -1)
				FctFromKeyboard(fct);
			else
			{
				switch (key)
				{
					case kKBD_ARROWLEFT:
						if (modifiers & B_CONTROL_KEY)
							fProtocole->StdCharFromKbd(0x7f);
						else
							fProtocole->StdCharFromKbd(0x08);
						break;

					case kKBD_ARROWDOWN: /* fleche bas */
						fProtocole->StdCharFromKbd(0x0a);
						break;

					case kKBD_ARROWRIGHT: /* fleche droite */
						fProtocole->StdCharFromKbd(0x09);
						break;

					case kKBD_ARROWUP: /* fleche haut */
						fProtocole->StdCharFromKbd(0x0b);
						break;

					case kKBD_PADENTER: /* enter */
						if (modifiers & B_SHIFT_KEY)
							fProtocole->StdCharFromKbd(0x1e);
						else if (modifiers & B_CONTROL_KEY)
							fProtocole->StdCharFromKbd(0x0c);
						else
							fProtocole->StdCharFromKbd(0x0d);
						break;

					default:
						if (ascii[0] >= 0x20)
							StdCharToVideotex(ascii[0]);
						else
							fProtocole->StdCharFromKbd(ascii[0]);
						break;
				}
			}
			break;

		case eMixteFr:
		case eMixteUS:
		case eTeletelEtendu:
		case eTeleInfoFr:
		case eTeleInfoUS:
			switch (key)
			{
				case kKBD_ARROWLEFT:
					if (modifiers & B_SHIFT_KEY)
						fProtocole->CsiCharFromKbd(0x50);
					else if (modifiers & B_CONTROL_KEY)
						fProtocole->StdCharFromKbd(0x7f);
					else
						fProtocole->CsiCharFromKbd(0x44);
					break;

				case kKBD_ARROWDOWN: /* fleche bas */
					if (modifiers & B_SHIFT_KEY)
						fProtocole->CsiCharFromKbd(0x4c);
					else
						fProtocole->CsiCharFromKbd(0x42);
					break;	/* RMP: I think a break is needed */

				case kKBD_ARROWRIGHT: /* fleche droite */
					if (modifiers & B_SHIFT_KEY) {
						fProtocole->CsiCharFromKbd(0x34);
						if (fInsert)
						{
							fInsert = 0;
							fProtocole->StdCharFromKbd(0x6c);
						}
						else
						{
							fInsert = 1;
							fProtocole->StdCharFromKbd(0x68);
						}
					}
					else
						fProtocole->CsiCharFromKbd(0x43);
					break;	/* RMP: I think a break is needed */

				case kKBD_ARROWUP: /* fleche haut */
					if (modifiers & B_SHIFT_KEY)
						fProtocole->CsiCharFromKbd(0x4d);
					else
						fProtocole->CsiCharFromKbd(0x41);
					break;	/* RMP: I think a break is needed */

				case kKBD_PADENTER: /* enter */
					if (modifiers & B_SHIFT_KEY)
						fProtocole->CsiCharFromKbd(0x48);
					else if (modifiers & B_CONTROL_KEY)
					{
						fProtocole->CsiCharFromKbd(0x32);
						fProtocole->StdCharFromKbd(0x4a);
					}
					else
						fProtocole->StdCharFromKbd(0x0d);
					break;

				default:
					if (((fMode == eTeleInfoFr) || (fMode == eTeleInfoUS)) && (ascii[0] == B_FUNCTION_KEY))
					{
						if ((key >= B_F1_KEY) && (key <= B_F9_KEY))
							fProtocole->InfoFctFromKbd(0x40+key-B_F1_KEY);
						/*
						else
							NXBeep();
						*/
					}
					else
					{
						if (ascii[0] >= 0x20)
						{
							switch (fMode)
							{
								default:
								case eTeletelEtendu:
									StdCharToVideotex(ascii[0]);
									break;

								case eTeleInfoFr:
								case eMixteFr:
									StdCharToFR(ascii[0]);
									break;

								case eTeleInfoUS:
								case eMixteUS:
									StdCharToUS(ascii[0]);
									break;
							}
						}
						else
							fProtocole->StdCharFromKbd(ascii[0]);
					}
					break;
			}
			break;
	}
}

//------------------------------------------------------------------------

void KeyboardModule::StdCharToFR(uint16 c)
{
	if (c > 31) 
	{
		switch (c) /* remapping caracteres speciaux non ascii 7bits */
		{
			case 0xa3: /* livre */
				fProtocole->StdCharFromKbd(0x23);
				break;

			case 0x88: /* a grave */
				fProtocole->StdCharFromKbd(0x40);
				break;

			case 0xa1: /* degre */
				fProtocole->StdCharFromKbd(0x5b);
				break;

			case 0x8d: /* c cedille */
				fProtocole->StdCharFromKbd(0x5c);
				break;

			case 0xa4: /* paragraphe */
				fProtocole->StdCharFromKbd(0x5d);
				break;

			case 0x8e: /* e aigu */
				fProtocole->StdCharFromKbd(0x7b);
				break;

			case 0x9d: /* u grave */
				fProtocole->StdCharFromKbd(0x7c);
				break;

			case 0x8f: /* e grave */
				fProtocole->StdCharFromKbd(0x7d);
				break;

			case 0xac: /* trema */
				fProtocole->StdCharFromKbd(0x7e);
				break;

			default:
				if (c <= 0x7f) 
				{
					if (fInvmajmin)
						inv_maj_min(&c);
					fProtocole->StdCharFromKbd(c);
				}
				break;
		}
	}
	else
	{
		/*
		NXBeep();
		*/
	}
}

//------------------------------------------------------------------------

void KeyboardModule::StdCharToUS(uint16 c)
{
	if ((c > 31) && (c <= 0x7f)) /* pas de remapping */
	{
		if (fInvmajmin)
			inv_maj_min(&c);
		fProtocole->StdCharFromKbd(c);
	}
	else
	{
		/*
		NXBeep();
		*/
	}
}

//------------------------------------------------------------------------

void KeyboardModule::StdCharToVideotex(uint16 c)
{
//	if ((c > 31) && (c < 127))
	if (c > 31) 
	{
		switch (c)
		{
			case '`' :	/* back quote */
				fProtocole->SpeCharFromKbd(0x41);
				break;

			case 0x88:	/* a grave */
				fProtocole->SpeCharFromKbd(0x41);
				fProtocole->StdCharFromKbd('a');
				break;

			case 0x8f:	/* e grave */
				fProtocole->SpeCharFromKbd(0x41);
				fProtocole->StdCharFromKbd('e');
				break;

			case 0x9d:	/* u grave */
				fProtocole->SpeCharFromKbd(0x41);
				fProtocole->StdCharFromKbd('u');
				break;

//
//			case '\'':	/* quote * /
//				fProtocole->SpeCharFromKbd(0x42);
//				break;
//

			case 0x8e:	/* e aigu */
				fProtocole->SpeCharFromKbd(0x42);
				fProtocole->StdCharFromKbd('e');
				break;

			case '^':	/* circumflex */
				fProtocole->SpeCharFromKbd(0x43);
				break;

			case 0x89:	/* a circumflex */
				fProtocole->SpeCharFromKbd(0x43);
				fProtocole->StdCharFromKbd('a');
				break;

			case 0x90:	/* e circumflex */
				fProtocole->SpeCharFromKbd(0x43);
				fProtocole->StdCharFromKbd('e');
				break;

			case 0x94:	/* i circumflex */
				fProtocole->SpeCharFromKbd(0x43);
				fProtocole->StdCharFromKbd('a');
				break;

			case 0x99:	/* o circumflex */
				fProtocole->SpeCharFromKbd(0x43);
				fProtocole->StdCharFromKbd('e');
				break;

			case 0x9e:	/* u circumflex */
				fProtocole->SpeCharFromKbd(0x43);
				fProtocole->StdCharFromKbd('u');
				break;

			case 0x8a:	/* a trema */
				fProtocole->SpeCharFromKbd(0x48);
				fProtocole->StdCharFromKbd('a');
				break;

			case 0x91:	/* e trema */
				fProtocole->SpeCharFromKbd(0x48);
				fProtocole->StdCharFromKbd('e');
				break;

			case 0x95:	/* i trema */
				fProtocole->SpeCharFromKbd(0x48);
				fProtocole->StdCharFromKbd('a');
				break;

			case 0x9a:	/* o trema */
				fProtocole->SpeCharFromKbd(0x48);
				fProtocole->StdCharFromKbd('e');
				break;

			case 0x9f:	/* u trema */
				fProtocole->SpeCharFromKbd(0x48);
				fProtocole->StdCharFromKbd('u');
				break;

			case 0x8d:	/* c cedille */
				fProtocole->SpeCharFromKbd(0x4b);
				fProtocole->StdCharFromKbd('c');
				break;

			case '[':
				fProtocole->StdCharFromKbd(0x5b);
				break;

			case ']':
				fProtocole->StdCharFromKbd(0x5d);
				break;

			default:
				if (c <= 0x7f) 
				{
					if (fInvmajmin)
						inv_maj_min(&c);
					fProtocole->StdCharFromKbd(c);
				}
				break;
		}
	}
	else
	{
		/*
		NXBeep();
		*/
	}
}
