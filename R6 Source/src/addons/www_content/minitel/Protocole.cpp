/*************************************************************************
/
/	Protocole.cpp
/
/	Modified by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#include "Protocole.h"
#include "Iso6429ScreenModule.h"
#include "KeyboardModule.h"
#include "MinitelView.h"
#include "ModemModule.h"
#include "StdScreenModule.h"
#include "VirtScreenModule.h"


//========================================================================

Protocole::Protocole(MinitelView* view)
	: fClavier				(NULL),
	  fModem				(NULL),
	  fCompatX3				(0),
	  fConnect				(0),
	  fTraitEcranIso6429	(NULL),
	  fAffiChageEcran		(view),
	  fTraitEcran			(NULL)
{
	Reset();
}

//------------------------------------------------------------------------

Protocole::~Protocole()
{
	delete fTraitEcranIso6429;
	delete fClavier;
	delete fModem;
	delete fTraitEcran;
}

//------------------------------------------------------------------------

void Protocole::DoOpen(entry_ref* ref)
{
	BFile	file(ref, O_RDONLY);
	off_t	size;

	if ((file.InitCheck() == B_NO_ERROR) && (file.GetSize(&size) == B_NO_ERROR))
	{
		char*	buffer;

		buffer = (char*)malloc(size);
		if (buffer)
		{
			file.Read(buffer, size);
			for (int32 i = 0; i < size; i++)
				CharFromModem(buffer[i]);
			free(buffer);
		}
	}
}

//------------------------------------------------------------------------

void Protocole::Reset()
{
	if (!fClavier)
	{
		fClavier = new KeyboardModule(this, fAffiChageEcran);
		fClavierOK = true;
	}
	fModeClavier = eTeletel;
	fClavier->SetMode(fModeClavier);

	if (!fModem)
		fModem = new ModemModule(this, fAffiChageEcran);
	fModemOK = true;

	delete fTraitEcranIso6429;
	fTraitEcranIso6429 = new Iso6429ScreenModule(this, fAffiChageEcran);

	delete fTraitEcran;
	fTraitEcran = new StdScreenModule(fAffiChageEcran);

	Mode40Col();
	StatusConnect(fConnect);
	fModeProt = 0;
}

//------------------------------------------------------------------------

void Protocole::ToggleCompatX3(int32 flag)
{
	fCompatX3 = flag;
}

//------------------------------------------------------------------------

void Protocole::StdCharFromKbd(unsigned char c)
{
	if (fClavierOK)
	{
		if (fConnect)
			fModem->CharToModem(c);
		else
			fScrProc->OutScr(c);
	}
}

//------------------------------------------------------------------------

void Protocole::SpeCharFromKbd(unsigned char c)
{
	if (fClavierOK)
	{
		fModem->CharToModem(0x19);
		fModem->CharToModem(c);
	}
}

//------------------------------------------------------------------------

void Protocole::CsiCharFromKbd(unsigned char c)
{
	if (fClavierOK)
	{
		fModem->CharToModem(0x1b);
		fModem->CharToModem(0x5b);
		fModem->CharToModem(c);
	}
}

//------------------------------------------------------------------------

void Protocole::TeletelFctFromKbd(unsigned char c)
{
	if (fClavierOK)
	{
		if (fCompatX3)
		{
			fModem->CharToModem(0x1b);
			fModem->CharToModem(c + 0x20);
			fModem->CharToModem(0x34);
			fModem->CharToModem(0x0d);
		}
		else
		{
			fModem->CharToModem(0x13);
			fModem->CharToModem(c + 0x40);
		}
	}
}

//------------------------------------------------------------------------

void Protocole::InfoFctFromKbd(unsigned char c)
{
	if (fClavierOK)
	{
		fModem->CharToModem(0x1b);
		fModem->CharToModem(0x4f);
		fModem->CharToModem(c);
	}
}

//------------------------------------------------------------------------

void Protocole::CharFromModem(unsigned char c)
{
	if (fModemOK)
	{
//fprintf(stderr,"Protocole::CharFromModem begin\n");
		switch (fModeProt)
		{
			case 0:	// mode normal
					if (c == 0x1b)	// ESC ?
					{
						fModeProt = 1;
					}
					else			// Non > ecran ...
					{
						fScrProc->OutScr(c);
					}
					break;

			case 1:	// mode ESC recu
					switch (c)
					{
						case 0x39:
							fModeProt = 10;
							break;

						case 0x3a:
							fModeProt = 20;
							break;

						case 0x3b:
							fModeProt = 30;
							break;

						case 0x61:
							fModeProt = 40;
							break;

						default:	// inconnu
							fModeProt = 0;
							fScrProc->OutScr(0x1b);
							fScrProc->OutScr(c);
							break;						
					}
					break;

			case 10:	// mode PRO 1 execution
				fModeProt = 0;
				Pro1(c);
				break;

			case 20:	// mode PRO 2 argument 1
				fModeProt = 21;
				fArg1 = c;
				break;

			case 21:	// mode PRO 2 execution
				fModeProt = 0;
				Pro2(fArg1, c);
				break;

			case 30:	// mode PRO 3 argument 1
				fModeProt = 31;
				fArg1 = c;
				break;

			case 31:	// mode PRO 3 argument 2
				fModeProt = 32;
				fArg1 = c;
				break;

			case 32:	// mode PRO 3 execution
				fModeProt = 0;
				Pro3(fArg1, fArg2, c);
				break;

			case 40:	// mode demande pos curseur
				fModeProt = 0;
				fModem->CharToModem(0x1f);	// Us
				fModem->CharToModem((unsigned char)fScrProc->GetLigne());
				fModem->CharToModem((unsigned char)fScrProc->GetColonne());
				break;

			case 100:	// mode transparence
				if (c != 0)
					fTranspCount--;
				if (fTranspCount <= 0)
					fModeProt = 0;
				break;

			case -1:	// protocole inhibe (mode teleinfo)
				fScrProc->OutScr(c);
				break;

			default:	// que se passe-t-il ????
				fModeProt = 0;
				fScrProc->OutScr(c);
				break;
		}
	}
}

//------------------------------------------------------------------------

void Protocole::Pro1(unsigned char c1)
// commandes protocole PRO 1
{
	unsigned char dum;
	
	if (c1 == 0x6c) // demande retournement >> echoue de tte facon
	{
		if (fCompatX3)
		{
			fModem->CharToModem(0x1b);
			fModem->CharToModem(0x21);
			fModem->CharToModem(0x35);
			fModem->CharToModem(0x0d);
		}
		else
		{
			fModem->CharToModem(0x13);
			fModem->CharToModem(0x51);
		}
	}
	else if (c1 == 0x6d) // demande retour vitesse stds
	{
		if (fCompatX3)
		{
			fModem->CharToModem(0x1b);
			fModem->CharToModem(0x21);
			fModem->CharToModem(0x35);
			fModem->CharToModem(0x0d);
		}
		else
		{
			fModem->CharToModem(0x13);
			fModem->CharToModem(0x51);
		}
	}
	else if (c1 == 0x70) // demande status terminal
	{
		dum = 0x4a; // bit 4:peripherique pret si 0
				// bits 3:detection porteuse si 1 ; 2:0 ;  1:vitesses stds si 1
				// bit 0:mode non oppose
		fModem->CharToModem(0x1b);	// Esc
		fModem->CharToModem(0x3a);	// Pro 2
		fModem->CharToModem(0x71);	// Reponse status terminal
		fModem->CharToModem(dum);	// status
	}
	else if (c1 == 0x72) // demande status fctmt
	{
		RepStatusPro2();
	}
	else if (c1 == 0x74) // demande status vitesse
	{
		dum = 0x4f; // PADX3 non valide ... acq multiple
		fModem->CharToModem(0x1b);	// Esc
		fModem->CharToModem(0x3a);	// Pro 2
		fModem->CharToModem(0x75);	// Reponse statut vitesse
		fModem->CharToModem(dum);	// status
	}
	else if (c1 == 0x76) // demande status protocole
	{
		dum = 0x4f; // PADX3 non valide ... acq multiple
		fModem->CharToModem(0x1b);	// Esc
		fModem->CharToModem(0x3a);	// Pro 2
		fModem->CharToModem(0x77);	// Reponse statut protocole
		fModem->CharToModem(dum);	// status
	}
	else if (c1 == 0x7b) // demande identif terminal
	{
		fModem->CharToModem(0x01);	// Soh
		fModem->CharToModem('Z');	// id1 : constructeur
		fModem->CharToModem('u');	// id2 : type modem Bi-std
		fModem->CharToModem('1');	// id3 : version logiciel
		fModem->CharToModem(0x04);	// Eot
	}
	else if (c1 == 0x7f) // reset terminal
	{
		Reset();
		// acquittement
		if (fCompatX3)
		{
			fModem->CharToModem(0x1b);
			fModem->CharToModem(0x2e);
			fModem->CharToModem(0x35);
			fModem->CharToModem(0x0d);
		}
		else
		{
			fModem->CharToModem(0x13);
			fModem->CharToModem(0x5e);
		}
	}
//	else
//	DPRINTF_P("commande protocole PRO 1 non supportee %x\n",(unsigned int)c1);
}

//------------------------------------------------------------------------

void Protocole::Pro2(unsigned char c1, unsigned char c2)
// commandes protocole PRO 2
{	
	if ((c1 == 0x71) || (c1 == 0x73) || (c1 == 0x75) || (c1 == 0x77))	
	{
		// filtrage des reponses
	}
	else
	{	
		if (c1 == 0x66) // transparence
		{
			if (fCompatX3)
			{
				fModem->CharToModem(0x1b);
				fModem->CharToModem(0x27);
				fModem->CharToModem(0x35);
				fModem->CharToModem(0x0d);
			}
			else
			{
				fModem->CharToModem(0x13);
				fModem->CharToModem(0x57);
			}
			fModeProt = 100;
			fTranspCount = (int32)c2;	
		}
		else if ((c1 == 0x69) && ((c2 == 0x44) || (c2 == 0x45))) 
			// demande start PCE, clavier
		{
			RepStatusPro2();
		}
		else if ((c1 == 0x6a) && ((c2 == 0x44) || (c2 == 0x45))) 
			// demande stop PCE, clavier
		{
			RepStatusPro2();
		}
		else if ((c1 == 0x72) && (c2 == 0x59)) // demande status clavier
		{
			RepStatusClavier();
		}
		else if ((c1 == 0x69) && (c2 == 0x43)) // start rouleau
		{
			fRlxFlag = 1;
			fScrProc->ModeRlx(1);
			RepStatusPro2();
		}
		else if ((c1 == 0x6a) && (c2 == 0x43)) // stop rouleau
		{
			fRlxFlag = 0;
			fScrProc->ModeRlx(0);
			RepStatusPro2();
		}
		else if ((c1 == 0x31) && (c2 == 0x7d)) // passage mode Videotex > Teleinfo
		{
			fModem->CharToModem(0x1b);
			fModem->CharToModem(0x5b);
			fModem->CharToModem(0x3f);
			fModem->CharToModem(0x7a);
			Mode80Col();
			fModeProt = -1; /* blocage protocole */
			fModeClavier = eTeleInfoUS;
			fClavier->SetMode(eTeleInfoUS);
		}
		else if ((c1 == 0x32) && (c2 == 0x7d)) // passage mode Videotex > Mixte
		{
			fModem->CharToModem(0x13);
			fModem->CharToModem(0x70);
			Mode80Col();
			fModeClavier = eMixteUS;
			fClavier->SetMode(eMixteUS);
		}
		else if ((c1 == 0x32) && (c2 == 0x7e)) // passage mode Mixte > Videotex
		{
// COMMENTED OUT March 23 93 (Pb INPI)
//			[modem CharToModem:0x13]; 
//			[modem CharToModem:0x71];
			Mode40Col();
			fModeClavier = eTeletel;
			fClavier->SetMode(eTeletel);
		}
//		else
//		DPRINTF_P("commande protocole PRO 2 non supportee %x %x\n",(unsigned int)c1, (unsigned int)c2);
	}
}

//------------------------------------------------------------------------

void Protocole::RepStatusPro2()
{
	unsigned char dum;
	
	dum = 0x40 | (2 * fRlxFlag + fModeEcran);
	fModem->CharToModem(0x1b);	// Esc
	fModem->CharToModem(0x3a);	// Pro 2
	fModem->CharToModem(0x73);	// Reponse statut fct
	fModem->CharToModem(dum);	// status
}

//------------------------------------------------------------------------

void Protocole::RepStatusClavier()
{
	unsigned char dum;
	
	dum = 0x40;
	if (fModeClavier == eTeletelEtendu)
		dum = 0x41;
	if (fModeClavier == eTeletelCo)
		dum = 0x44;
	
	fModem->CharToModem(0x1b);	// Esc
	fModem->CharToModem(0x3b);	// Pro 3
	fModem->CharToModem(0x73);	// Reponse statut fct
	fModem->CharToModem(0x59);	// Clavier
	fModem->CharToModem(dum);	// status
}

//------------------------------------------------------------------------

void Protocole::Pro3(unsigned char c1, unsigned char c2, unsigned char c3)
// commandes protocole PRO 3
{
	if ((c1 == 0x69) && (c2 == 0x59) && (c3 == 0x41)) // passage clavier etendu
	{
		if (fModeClavier <= eTeletelCo)
		{
			fModeClavier = eTeletelEtendu;
			fClavier->SetMode(fModeClavier);
		}
		RepStatusClavier();
	}
	else if ((c1 == 0x69) && (c2 == 0x59) && (c3 == 0x43)) // passage clavier C0
	{
		if (fModeClavier <= eTeletelCo)
		{
			fModeClavier = eTeletelCo;
			fClavier->SetMode(fModeClavier);
		}
		RepStatusClavier();
	}
	else if ((c1 == 0x6a) && (c2 == 0x59) && (c3 == 0x41)) // stop clavier etendu
	{
		if ((fModeClavier == eTeletelCo) || (fModeClavier == eTeletelEtendu))
		{
			fModeClavier = eTeletel;
			fClavier->SetMode(fModeClavier);
		}
		RepStatusClavier();
	}
	else if ((c1 == 0x69) && (c2 == 0x59) && (c3 == 0x43)) // stop clavier C0
	{
		if (fModeClavier == eTeletelCo)
		{
			fModeClavier = eTeletelEtendu;
			fClavier->SetMode(fModeClavier);
		}
		RepStatusClavier();
	}
//	else
//	DPRINTF_P("commande protocole PRO 3 non supportee %x %x %x\n", (unsigned int)c1, (unsigned int)c2, (unsigned int)c3);
}

//------------------------------------------------------------------------

void Protocole::ConnexionFin()
{
	if (fCompatX3)
	{
		fModem->CharToModem(0x1b);
		fModem->CharToModem(0x2e);
		fModem->CharToModem(0x34);
		fModem->CharToModem(0x0d);
	}
	else
	{
		fModem->CharToModem(0x13);
		fModem->CharToModem(0x49);
	}
}

//------------------------------------------------------------------------

void Protocole::StatusConnect(int32 stat)
{
	fConnect = stat;
	fScrProc->SetConnectStat(stat);
}

//------------------------------------------------------------------------

void Protocole::Mode40Col()
{
	fModeEcran = 0;
	fRlxFlag = 0;
	fScrProc = fTraitEcran;
	fScrProc->Reset();
}

//------------------------------------------------------------------------

void Protocole::Mode80Col()
{
	fModeEcran = 1;
	fRlxFlag = 1;
	fScrProc = fTraitEcranIso6429;
	fScrProc->Reset();
}

//------------------------------------------------------------------------

void Protocole::RetourFrom80Col()
{
	Reset();
	// acquittement
	fModem->CharToModem(0x13);
	fModem->CharToModem(0x5e);
}

//------------------------------------------------------------------------

void Protocole::Goto80Col()
{
	Mode80Col();
	fModeClavier = eMixteUS;
	fClavier->SetMode(fModeClavier);
}

//------------------------------------------------------------------------

void Protocole::JeuAmericain()
{
	if (fModeClavier == eMixteFr)
		fModeClavier = eMixteUS;
	if (fModeClavier == eTeleInfoFr)
		fModeClavier = eTeleInfoUS;
	fClavier->SetMode(fModeClavier);
}

//------------------------------------------------------------------------

void Protocole::JeuFrancais()
{
	if (fModeClavier == eMixteUS)
		fModeClavier = eMixteFr;
	if (fModeClavier == eTeleInfoUS)
		fModeClavier = eTeleInfoFr;
	fClavier->SetMode(fModeClavier);
}

//------------------------------------------------------------------------

void Protocole::EventFromKeyboard(const char* ascii, int32 modifiers, int32 key)
{
	fClavier->EventFromKeyboard(ascii, modifiers, key);
}
