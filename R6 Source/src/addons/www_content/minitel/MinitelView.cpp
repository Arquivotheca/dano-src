/*************************************************************************
/
/	MinitelView.cpp
/
/	Written by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#include <stdlib.h>
#include <Window.h>
#include <Screen.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Path.h>
#include <FindDirectory.h>
#include <File.h>
#include <NodeInfo.h>
#include <Button.h>
#include <MenuBar.h>

#include "MinitelView.h"
#include "MinitelFont.h"
#include "KeyboardModule.h"
#include "ModemModule.h"
#include "Protocole.h"


#define kDEFAULT_FONT	"ProFontISOLatin1"
#define kPLUGIN_NAME	"Minitel"


enum	KB_MESSAGES
{
	eButtonConnect = 1000,
	eButtonSummary,
	eButtonGuide,
	eButtonRepeat,
	eButtonCorrect,
	eButtonBack,
	eButtonEnter,
	eButtonCancel,
	eButtonNext,
	eTextPhone,
	ePhoneModified,
	eMenuNumber,
	eMenuAdd,
	eMenuRemove,
};


struct	connect_data
{
	ModemModule*	modem;
	char*			number;
};


//========================================================================

static void empty_car(struct mem_car *caractere)
{
	caractere->col_fnd = eNoir;
	caractere->col_txt = eBlanc;
	caractere->pol = kG0_NORM;
	caractere->inv = false;
	caractere->masq = false;
	caractere->soul = false;
	caractere->delim = 2;
	caractere->car = ' ';
}

//------------------------------------------------------------------------

static int get_haut(int32 pol)
{
	int32	resu = 1;
	
	switch (pol)
	{
		case kG0_NORM:
			resu = 1;
			break;

		case kG0_DBHAUT: 
			resu = 2;
			break;

		case kG0_DBLARG: 
			resu = 1;
			break;

		case kG0_DBTAIL:
			resu = 2;
			break;

		case kG1_NORM + kDSJT_OFF:
		case kG1_NORM:
			resu = 1;
			break;
	}
	return resu;
}

//------------------------------------------------------------------------

static int32 get_larg(int32 pol)
{
	int32	resu = 1;
	
	switch (pol)
	{
		case kG0_NORM:
			resu = 1;
			break;

		case kG0_DBHAUT: 
			resu = 1;
			break;

		case kG0_DBLARG: 
			resu = 2;
			break;

		case kG0_DBTAIL:
			resu = 2;
			break;

		case kG1_NORM + kDSJT_OFF:
		case kG1_NORM:
			resu = 1;
			break;
	}
	return resu;
}


//========================================================================

MinitelView::MinitelView(BRect rect)
	: BView			(rect,
					 kPLUGIN_NAME,
					 B_FOLLOW_NONE,
					 B_WILL_DRAW
					 | B_PULSE_NEEDED),
	  fDraw			(true),
	  fSelBox		(0),
	  fCursorFlag	(0),
	  fCursorState	(kCURSOR_PAUSE),
	  fMaxCol		(40),
	  fOldStatus	(0),
	  fCharMask		(NULL),
	  fCharMaskDH	(NULL),
	  fCharMaskDS	(NULL),
	  fCharMaskDW	(NULL),
	  fCharTable	(NULL),
	  fProtocole	(NULL)
{
#if 0
#include <TranslationUtils.h>
	uchar*		bits;
	BBitmap*	newfont;
	BBitmap*	font;
	BView*		view;

  	newfont = BTranslationUtils::GetBitmap("/boot/projects/iad/src/addons/www_content/minitel/typo-raouf.png");
	font = new BBitmap(newfont->Bounds(), B_CMAP8, true);
	view = new BView(font->Bounds(), "font", 0, 0);
	font->AddChild(view);
	font->Lock();
	view->DrawBitmap(newfont);
	font->Unlock();

	bits = (uchar*)font->Bits();
	for (int32 loop = 0; loop < font->BitsLength(); loop++)
	{
		if ((loop) && !(loop & 0xf))
			printf("\n");
		if (*bits++ == 0)
			printf("0x3f, ");
		else
			printf("0x00, ");
	}
#endif
	Show();
}

//------------------------------------------------------------------------

MinitelView::~MinitelView()
{
	thread_id	thread;

	if ((thread = find_thread("minitel_connect")) != B_NAME_NOT_FOUND)
		kill_thread(thread);

#ifdef USE_PREFERENCE_PANEL
	fPreferenceWindow->Lock();
	fPreferenceWindow->Quit();
#endif

	delete(fProtocole);
}

//------------------------------------------------------------------------

void MinitelView::AllAttached()
{
#ifdef USE_PREFERENCE_PANEL
	fPreferenceWindow = new PreferenceWindow(this);
#endif
}

//------------------------------------------------------------------------

void MinitelView::AttachedToWindow()
{
	SetStdColor();
	SetCellSize(kFONT_CELL_WIDTH, kFONT_CELL_HEIGHT);
	MakeFocus(true);
	fProtocole = new Protocole(this);
}

//------------------------------------------------------------------------

void MinitelView::Draw(BRect rect)
{
	if (fDraw)
	{
		if (LockLooper())
		{
			int32 l;
//			BRect rect;

//DPRINTF_TV("debut drawself\n");
			if (fCursorFlag)
				HideCursor(true);
//			fond ecran eNoir
//			couleur(eNoir);
//			rect.left = 0;
//			rect.top = 0;
//			rect.right = rect.left + fRapLarg*fCsteTaille*fMaxCol;
//			rect.bottom = rect.top + fCsteTaille*(kMAX_LIG + 1); // +1: kMAX_LIG lines of text and one of status
//			FillRect(rect);

// fond caracteres
//			for (l = 0; l <= kMAX_LIG; l++)
//				DrawBack(l, 1, fMaxCol);

// caracteres
//			for (l = 1; l <= kMAX_LIG; l++)
//				DrawText(l, 1, fMaxCol);
//			DrawRangee0(fOldStatus);


			if (fCharMask)
			{
//				DrawRangee0(fOldStatus);
//				for (l = 1; l <= kMAX_LIG; l++)
//					DrawArea(l, 1, fMaxCol);
				int32 cbeg = (int32)(rect.left / fCX + 1);
				int32 cend = (int32)(rect.right / fCX + 1);
				int32 lbeg = (int32)(rect.top / fCY);
				int32 lend = (int32)(rect.bottom / fCY);
				if (lbeg < 0)
					lbeg = 0;
				if (lbeg > kMAX_LIG)
					lbeg = kMAX_LIG;
				if (lend < 0)
					lend = 0;
				if (lend > kMAX_LIG)
					lend = kMAX_LIG;
				if (cbeg <= 0)
					cbeg = 1;
				if (cbeg > fMaxCol)
					cbeg = fMaxCol;
				if (cend <= 0)
					cend = 1;
				if (cend > fMaxCol)
					cend = fMaxCol;
				if (lbeg == 0)
				{
					DrawRangee0(fOldStatus);
					lbeg = 1;
				}
				for (l = lbeg; l <= lend; l++)
					DrawArea(l, cbeg, cend);
			}
			else
			{
				Couleur(eNoir);
				FillRect(rect);
			}
			if (fCursorFlag)
				HideCursor(false);
			UnlockLooper();
		}
	}
}

//------------------------------------------------------------------------

void MinitelView::KeyDown(const char* bytes, int32 num_bytes)
{
	int32		key;
	int32		modifiers;
	BMessage*	msg = Window()->CurrentMessage();

	msg->FindInt32("key", &key);
	msg->FindInt32("modifiers", &modifiers);
	fProtocole->EventFromKeyboard(bytes, modifiers, key);
}

//------------------------------------------------------------------------

void MinitelView::MouseDown(BPoint where)
{
	MakeFocus(true);
}

//------------------------------------------------------------------------

void MinitelView::Pulse()
{
	AffiCursor();
}

//------------------------------------------------------------------------

void MinitelView::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case eButtonConnect:
		case eTextPhone:
			{
				if (fProtocole->fModem->fStatus == kSTATUS_DISCONNECT)
				{
					const char*		number;
					connect_data*	data;
					thread_id		id;

					data = new connect_data();
					msg->FindString("number", &number);
					data->number = (char *)malloc(strlen(number) + 1);
					strcpy(data->number, number);
					data->modem = fProtocole->fModem;
					id = spawn_thread(ConnectThread, "minitel_connect", B_NORMAL_PRIORITY, (void *)data);
					if (id > 0)
						resume_thread(id);
					else
					{
						free(data->number);
						delete data;
					}
				}
				else if ((fProtocole->fModem->fStatus == kSTATUS_CONNECT) &&
						 (msg->what != eTextPhone))
					fProtocole->fClavier->Connexion();
			}
			break;

		case eButtonSummary:
			fProtocole->fClavier->Sommaire();
			break;

		case eButtonGuide:
			fProtocole->fClavier->Guide();
			break;

		case eButtonRepeat:
			fProtocole->fClavier->Repetition();
			break;

		case eButtonCorrect:
			fProtocole->fClavier->Correction();
			break;

		case eButtonBack:
			fProtocole->fClavier->Retour();
			break;

		case eButtonEnter:
			fProtocole->fClavier->Envoi();
			break;

		case eButtonCancel:
			fProtocole->fClavier->Annulation();
			break;

		case eButtonNext:
			fProtocole->fClavier->Suite();
			break;

		case kDISCONNECT:
			fProtocole->fModem->Disconnect();
			break;

		case B_SIMPLE_DATA:
			{
				entry_ref	ref;
		
				if (msg->FindRef("refs", &ref) == B_NO_ERROR)
					fProtocole->DoOpen(&ref);
			}
			break;

		default:
			BView::MessageReceived(msg);
	}
}

//------------------------------------------------------------------------

void MinitelView::Affiche(unsigned char c)
// demande affichage caractere c pos crte
{
	if ((c < 32) || (c > 255))
	{
		//fprintf(stderr, "TVIEW : caractere non conforme %d (0x%x)\n",c,(unsigned int)c);
		return;
	}
	if (fDraw)
	{
//	if (fSelBox) effaceOldBox();
		Window()->Lock();
		if (fCursorFlag)
			HideCursor(true);
	}

// restore caracteres voisins si taille differente ancienne
	if ((get_haut(fMemPages[fCurLig][fCurCol].pol) == 2) && (get_haut(fCurPol) != 2) && (fMemPages[fCurLig-1][fCurCol].car == '\0'))
	{
		fMemPages[fCurLig - 1][fCurCol].pol = kG0_NORM;
		fMemPages[fCurLig - 1][fCurCol].car = ' ';
		DoAff(fMemPages[fCurLig - 1][fCurCol], fCurLig - 1, fCurCol);
		if ((get_larg(fMemPages[fCurLig][fCurCol].pol) == 2) && (get_larg(fCurPol) != 2))
		{
			fMemPages[fCurLig - 1][fCurCol + 1].pol = kG0_NORM;
			fMemPages[fCurLig - 1][fCurCol + 1].car = ' ';
			DoAff(fMemPages[fCurLig - 1][fCurCol + 1], fCurLig - 1, fCurCol + 1);
		}
	}
	if ((get_larg(fMemPages[fCurLig][fCurCol].pol) == 2) && (get_larg(fCurPol) != 2) && (fMemPages[fCurLig][fCurCol + 1].car == '\0'))
	{
		fMemPages[fCurLig][fCurCol + 1].pol = kG0_NORM;
		fMemPages[fCurLig][fCurCol + 1].car = ' ';
		DoAff(fMemPages[fCurLig][fCurCol + 1], fCurLig, fCurCol + 1);
	}
// affecte nvelle caracteristiques
	fMemPages[fCurLig][fCurCol].col_txt = fCurTxt;
	fMemPages[fCurLig][fCurCol].pol = fCurPol;
	fMemPages[fCurLig][fCurCol].inv = fInverseFlag;
	fMemPages[fCurLig][fCurCol].car = c;

// etat delimiteur (pas, a valider, a ecraser)
	switch (fMemPages[fCurLig][fCurCol].delim)
	{
		case 0:
			break;

		case 1:
			// DPRINTF_TV("Valid delim..\n"); 
			fMemPages[fCurLig][fCurCol].delim = 2;  // valide le delimiteur
			break;

		default:
		case 2:
			// DPRINTF_TV("restore att\n");
			fMemPages[fCurLig][fCurCol].delim = 0;
			RestoreAtt();	// delimiteur invalide
			break;
	}
	
// support doubles	
	if (get_larg(fCurPol) != 1)
	{
		if (fCurCol + 1 <= fMaxCol) 
		{
			fMemPages[fCurLig][fCurCol + 1] = fMemPages[fCurLig][fCurCol];
  			fMemPages[fCurLig][fCurCol + 1].pol = kG0_NORM;
  			fMemPages[fCurLig][fCurCol + 1].soul = 0;
			if (c == ' ')
				fMemPages[fCurLig][fCurCol + 1].car = ' '; // si space pas besoin de traitmt
  			else
  				fMemPages[fCurLig][fCurCol + 1].car = '\0'; // marqueur de double
		}
	}
	if (get_haut(fCurPol) != 1)
	{
		if (fCurLig - 1 >= 1)
		{
			fMemPages[fCurLig - 1][fCurCol] = fMemPages[fCurLig][fCurCol];
			fMemPages[fCurLig - 1][fCurCol].pol = kG0_NORM;
			fMemPages[fCurLig - 1][fCurCol].soul = 0;
			if (c == ' ')
				fMemPages[fCurLig - 1][fCurCol].car = ' ';
  			else
  				fMemPages[fCurLig - 1][fCurCol].car = '\0';
			if (get_larg(fCurPol) != 1)
			{
				if (fCurCol + 1 <= fMaxCol) 
				{
					fMemPages[fCurLig - 1][fCurCol + 1] = fMemPages[fCurLig][fCurCol];
					fMemPages[fCurLig - 1][fCurCol + 1].pol = kG0_NORM;
					fMemPages[fCurLig - 1][fCurCol + 1].soul = 0;
					if (c == ' ')
						fMemPages[fCurLig - 1][fCurCol + 1].car = ' ';
  					else
  						fMemPages[fCurLig - 1][fCurCol + 1].car = '\0';
				}
			}
		}
	}
	
// graphique disjoint
 	if ((fSoulFlag) && (fCurPol == kG1_NORM))
	{
//		DPRINTF_TV("graphique disjt\n");
		fMemPages[fCurLig][fCurCol].pol = kG1_NORM + kDSJT_OFF;
		if (fMemPages[fCurLig][fCurCol].car == 0x7f)
			fMemPages[fCurLig][fCurCol].car = 0x5f;
	}
	
// affichage proprement dit
	DoAff(fMemPages[fCurLig][fCurCol]);
	
// si espace dble haut et/ou large inutile de le considerer comme dble par la suite
	if ((c == ' ') && (fCurPol > kG0_NORM) && (fCurPol < kG1_NORM))
		fMemPages[fCurLig][fCurCol].pol = kG0_NORM;

// colonne courante
	fCurCol = fCurCol + get_larg(fCurPol);
	if (fCurCol > fMaxCol)
		fCurCol = fMaxCol;
	
	if (fDraw)
	{
		if (fCursorFlag)
			HideCursor(false);
		Window()->Unlock();
	}
}

//------------------------------------------------------------------------

void MinitelView::AfficheBrut(unsigned char c)
// demande affichage caractere c pos crte
{
	if ((c < 32) || (c > 255))
	{
		//fprintf(stderr, "TVIEW : caractere non conforme %d (0x%x)\n",c,(unsigned int)c);
		return;
	}
//	if (fSelBox) effaceOldBox();
	fMemPages[fCurLig][fCurCol].col_txt = fCurTxt;
	fMemPages[fCurLig][fCurCol].col_fnd = fCurFnd;
	fMemPages[fCurLig][fCurCol].pol = fCurPol;
	fMemPages[fCurLig][fCurCol].inv = fInverseFlag;
	fMemPages[fCurLig][fCurCol].soul = fSoulFlag;
	fMemPages[fCurLig][fCurCol].car = c;
	fMemPages[fCurLig][fCurCol].delim = 0;

	if (fDraw)
	{
		Window()->Lock();
		if (fCursorFlag)
			HideCursor(true);
	
		DoAff(fMemPages[fCurLig][fCurCol]);
	
		fCurCol = fCurCol + get_larg(fCurPol);
		if (fCurCol > fMaxCol)
			fCurCol = fMaxCol;
		if (fCursorFlag)
			HideCursor(false);
		Window()->Unlock();
		if (fCurLig == 0)
			DrawRangee0(fOldStatus);
	}
}

//------------------------------------------------------------------------

void MinitelView::AfficheCount(unsigned char c, int32 nbre)
// affichage avec repetition mode teletel
{
	int32	i;
	int32	c_max;
	int32	c_rep;
	int32	dummy_for_delim;
	int32	eff_width;
	int32	eff_height;
	struct	mem_car caractere;
	
	if ((c < 32) || (c > 255))
	{
		//fprintf(stderr, "TVIEW : caractere non conforme %d (0x%x)\n",c,(unsigned int)c);
		return;
	}
	
// prepare draws
	if (fDraw)
	{
//	if (fSelBox) effaceOldBox();
		Window()->Lock();
		if (fCursorFlag)
			HideCursor(true);
	}

// *** real stuff is here ***
	eff_width = get_larg(fCurPol); /* doubles largeur */
	eff_height = get_haut(fCurPol); /* doubles hauteur */
	c_rep = ((fCurCol + (nbre * eff_width) - 1 < fMaxCol) ?
		(fCurCol + (nbre * eff_width) - 1) : fMaxCol);

	dummy_for_delim = fMemPages[fCurLig][fCurCol].delim;

	caractere.delim = 0;
	if (fCurPol > kG0_DBTAIL)
		caractere.delim = 2;
	
	caractere.col_txt = fCurTxt;
	caractere.pol = fCurPol;
	caractere.inv = fInverseFlag;
	caractere.car = c;
	caractere.col_fnd = eNoir;
	caractere.masq = 0;
	caractere.soul = 0;
	if (dummy_for_delim == 1)
	{
//		DPRINTF_TV("delim explicite valide\n");
		caractere.col_fnd = fMemPages[fCurLig][fCurCol].col_fnd;
		caractere.masq = fMemPages[fCurLig][fCurCol].masq;
		caractere.soul = fMemPages[fCurLig][fCurCol].soul;
	}
	if ((fCurCol > 1) && (dummy_for_delim != 1))
	{
		caractere.col_fnd = fMemPages[fCurLig][fCurCol - 1].col_fnd;
		caractere.masq = fMemPages[fCurLig][fCurCol - 1].masq;
		caractere.soul = fMemPages[fCurLig][fCurCol - 1].soul;
	}

/* graphique disjoint */
 	if ((fSoulFlag) && (caractere.pol == kG1_NORM))
 	{
//		DPRINTF_TV("graphique disjt\n");
		caractere.pol = kG1_NORM + kDSJT_OFF;
		if (caractere.car == 0x7f)
			caractere.car = 0x5f;
	}

/* affectation des nvx caracteres */
	for (i = fCurCol; i <= c_rep; i = i + eff_width)
	{
	/* 1er cas : on ecrase un double haut */
		if ((get_haut(fMemPages[fCurLig][i].pol) == 2) && (eff_height != 2))
		{
			if (fMemPages[fCurLig - 1][i].car == '\0')
			{
				fMemPages[fCurLig - 1][i].pol = kG0_NORM;
				fMemPages[fCurLig - 1][i].car = ' ';
			}
			DoAff(fMemPages[fCurLig - 1][i], fCurLig - 1, i);
		}
	/* on affecte le nouveau caractere */
		fMemPages[fCurLig][i] = caractere;
	/* si double ou dble haut on recopie dans la cellule du dessus */
		if ((eff_height == 2) && (fCurLig > 1))
		{
			fMemPages[fCurLig - 1][i] = caractere;
			fMemPages[fCurLig - 1][i].pol = kG0_NORM;
			fMemPages[fCurLig - 1][i].soul = 0;
			if (caractere.car == ' ')
				fMemPages[fCurLig - 1][i].car = ' ';
			else
				fMemPages[fCurLig - 1][i].car = '\0';
		}
	/* si double ou dble large on recopie dans la cellule suivante et evtlmt celle du dessus */
		if ((eff_width == 2) && (i < fMaxCol))
		{
			fMemPages[fCurLig][i + 1] = caractere;
			fMemPages[fCurLig][i + 1].pol = kG0_NORM;
			fMemPages[fCurLig][i + 1].soul = 0;
			if (caractere.car == ' ')
				fMemPages[fCurLig][i + 1].car = ' ';
			else
				fMemPages[fCurLig][i + 1].car = '\0';
			if ((eff_height == 2) && (fCurLig > 1))
				fMemPages[fCurLig - 1][i + 1] = fMemPages[fCurLig][i + 1];
		}
	}

/* cas delimiteur a valider */
	if (dummy_for_delim == 1)
		fMemPages[fCurLig][fCurCol].delim = 2;

// first : ? extend of modification of area attributes
	c_max = c_rep ;
	while (c_max < fMaxCol)
	{
		if ((fMemPages[fCurLig][c_max + eff_width].delim == 0) &&
				((fMemPages[fCurLig][c_max+eff_width].col_fnd != caractere.col_fnd)
				|| (fMemPages[fCurLig][c_max+eff_width].masq != caractere.masq)
				|| (fMemPages[fCurLig][c_max+eff_width].soul != caractere.soul)))
			{
			c_max = c_max + eff_width;
			fMemPages[fCurLig][c_max].col_fnd = caractere.col_fnd;
			if (fCurPol <= kG0_DBTAIL)
			{
				fMemPages[fCurLig][c_max].masq = caractere.masq;
				fMemPages[fCurLig][c_max].soul = caractere.soul;
			}
		}
		else
			break;
	}
//	DPRINTF_TV("etendue modif fond : %d col_fnd %d col_txt %d inv %d\n", c_max, fMemPages[fCurLig][fCurCol].col_fnd, fMemPages[fCurLig][fCurCol].col_txt, caractere.inv);

// second : draw modified chars
//	redrawArea(fCurLig, fCurCol, c_max, caractere);
	DrawArea(fCurLig, fCurCol, c_max);

/* si double, dble haut ou dble large et space il peut etre consid�r� par la suite comme simple */
	if (((eff_width == 2) || (eff_height == 2)) && (caractere.car == ' '))
		for (i = fCurCol; i <= c_rep; i++)
			fMemPages[fCurLig][i].pol = kG0_NORM;

// calc current column
	fCurCol = fCurCol + get_larg(fCurPol) * nbre;
	if (fCurCol > fMaxCol)
		fCurCol = fMaxCol;

	if (fDraw)
	{
// end of draw
		if (fCursorFlag)
			HideCursor(false);
		Window()->Unlock();
	}
}

//------------------------------------------------------------------------

void MinitelView::DelCar(int32 coldeb, int32 nbre)
{
	int32	c;
	
	if (coldeb + nbre > fMaxCol)
		nbre = fMaxCol - coldeb;
	for (c = coldeb; c < coldeb + nbre; c++)
	{
		fMemPages[fCurLig][c] = fMemPages[fCurLig][c+nbre];
	}
	if (fDraw)
	{
		Window()->Lock();
	//	DrawBack(fCurLig, coldeb, fMaxCol);
	//	DrawText(fCurLig, coldeb, fMaxCol);
		DrawArea(fCurLig, coldeb, fMaxCol);
		Window()->Unlock();
	}
	for (c = coldeb + nbre; c <= fMaxCol; c++)
	{
		empty_car(&fMemPages[fCurLig][c]);
	}
}

//------------------------------------------------------------------------

void MinitelView::Efface()
{
	BRect	rect;

	Reset();
	if (fDraw)
	{
		Window()->Lock();
		Couleur(eNoir);
		SetPageRevele(false);
		rect.left = 0;
		rect.top = 0;
		rect.right = rect.left + fRapLarg * fCsteTaille * fMaxCol;
		rect.bottom = rect.top + fCsteTaille * (kMAX_LIG + 1); //+1 since there's also a status line
		FillRect(rect);
		DrawRangee0(fOldStatus);
		Window()->Unlock();
	}
}

//------------------------------------------------------------------------

void MinitelView::EffaceDebEcran()
{
	EffaceLignes(kMIN_LIG, fCurLig-1);
	EffaceDebLigne();
}

//------------------------------------------------------------------------

void MinitelView::EffaceDebLigne()
{
	register int32	c;
	BRect			rect;
	int32			eff_height;

	eff_height = 1;
	/* support effacement doubles hauteur */
	for (c = fCurCol; c <= fMaxCol; c++)
	{
		if (get_haut(fMemPages[fCurLig][c].pol) == 2)
		{
			eff_height = 2;
			break;
		}
	}
	if (fDraw)
	{
		Window()->Lock();
		if (fCursorFlag)
			HideCursor(true);
		Couleur(eNoir);
		rect.left = 0;
		rect.right = rect.left + fRapLarg * fCsteTaille * (fCurCol - kMIN_COL);
		if (eff_height == 1)
		{
			rect.top = fCsteTaille * fCurLig;
			rect.bottom = rect.top + fCsteTaille;
		}
		else
		{
			rect.top = fCsteTaille * (fCurLig - 1);
			rect.bottom = rect.top + fCsteTaille * 2;
		}
		FillRect(rect);
		if (fCursorFlag)
			HideCursor(false);
		Window()->Unlock();
	}
	if ((eff_height == 2) && (fCurLig > 1))
	{
		for (c = fCurCol; c <= fMaxCol; c++)
		{
			empty_car(&fMemPages[fCurLig][c]);
			empty_car(&fMemPages[fCurLig-1][c]);
		}
	}
	else
	{
		for (c = fCurCol; c <= fMaxCol; c++)
			empty_car(&fMemPages[fCurLig][c]);
	}
}

//------------------------------------------------------------------------

void MinitelView::EffaceFinEcran()
{
	EffaceFinLigne();
	EffaceLignes(fCurLig + 1, kMAX_LIG);
}

//------------------------------------------------------------------------

void MinitelView::EffaceFinLigne()
{
	register int32	c;
	BRect			rect;
	int32			eff_height;

	eff_height = 1;
	/* support effacement doubles hauteur */
	for (c = fCurCol; c <= fMaxCol; c++)
	{
		if (get_haut(fMemPages[fCurLig][c].pol) == 2)
		{
			eff_height = 2;
			break;
		}
	}
	if (fDraw)
	{
		Window()->Lock();
		if (fCursorFlag)
			HideCursor(true);
		Couleur(eNoir);
		rect.left = fRapLarg * fCsteTaille * (fCurCol - 1);
		rect.right = rect.left + fRapLarg * fCsteTaille * (fMaxCol - fCurCol + 1);
		if (eff_height == 1)
		{
			rect.top = fCsteTaille * fCurLig;
			rect.bottom = rect.top + fCsteTaille;
		}
		else
		{
			rect.top = fCsteTaille * (fCurLig - 1);
			rect.bottom = rect.top + fCsteTaille * 2;
		}
		FillRect(rect);
		if (fCursorFlag)
			HideCursor(false);
		Window()->Unlock();
	}
	if ((eff_height == 2) && (fCurLig > 1))
	{
		for (c = fCurCol; c <= fMaxCol; c++)
		{
			empty_car(&fMemPages[fCurLig][c]);
			empty_car(&fMemPages[fCurLig-1][c]);
		}
	}
	else
	{
		for (c = fCurCol; c <= fMaxCol; c++)
			empty_car(&fMemPages[fCurLig][c]);
	}
	if (fCurLig == 0)
		DrawRangee0(fOldStatus);
}

//------------------------------------------------------------------------

void MinitelView::EffaceLignes(int32 lignedeb, int32 lignefin)
{
	register int32	l;
	register int32	c;
	BRect			rect;

	if (lignefin >= lignedeb)
	{
		for (l = lignedeb; l <= lignefin; l++)
			for (c = kMIN_COL; c <= fMaxCol; c++)
				empty_car(&fMemPages[l][c]);

		if (fDraw)
		{
			Window()->Lock();
			if (fCursorFlag)
				HideCursor(true);
			Couleur(eNoir);
			rect.left = 0;
			rect.top = fCsteTaille * lignedeb;
			rect.right = rect.left + fRapLarg * fCsteTaille * (fMaxCol - kMIN_COL + 1);
			rect.bottom = rect.top + fCsteTaille * (lignefin - lignedeb + 1);
			FillRect(rect);
			if (fCursorFlag)
				HideCursor(false);
			Window()->Unlock();
		}
	}
	if (lignedeb == 0)
		DrawRangee0(fOldStatus);
}

//------------------------------------------------------------------------

void MinitelView::EnableCursor(bool enable)
{
	if (enable)
	{
		fCursorFlag = kCURSOR_ON;
		HideCursor(false);
	}
	else
	{
		HideCursor(true);
		fCursorFlag = kCURSOR_OFF;
	}
}

//------------------------------------------------------------------------

void MinitelView::Init40Col()
{
	Window()->Lock();
	fRapLarg = fRapLarg * (fMaxCol / 40); /* re-appel accidentel => meme largeur */
	fMaxCol = 40;
	Reset();
	if (fCursorFlag)
		HideCursor(true);
	SetCellSize(kFONT_CELL_WIDTH, kFONT_CELL_HEIGHT); //fCX, (10 * fCX) / 8);
	if (fCursorFlag)
		HideCursor(false);
	Window()->Unlock();
}

//------------------------------------------------------------------------

void MinitelView::Init80Col()
{
	Window()->Lock();
	fRapLarg = fRapLarg / (80 / fMaxCol); /* re-appel accidentel => meme largeur */
	fMaxCol = 80;
	Reset();
	if (fCursorFlag)
		HideCursor(true);
	SetCellSize(fCX, (10 * fCX) / 6);
	if (fCursorFlag)
		HideCursor(false);
	Window()->Unlock();
}

//------------------------------------------------------------------------

void MinitelView::InsCar(int32 coldeb, int32 nbre)
{
	int32	c;
	
	if (coldeb + nbre > fMaxCol)
		nbre = fMaxCol - coldeb;
	for (c = fMaxCol; c >= coldeb + nbre; c--)
	{
		fMemPages[fCurLig][c] = fMemPages[fCurLig][c-nbre];
	}
	if (fDraw)
	{
		Window()->Lock();
	//	DrawBack(fCurLig, coldeb, fMaxCol);
	//	DrawText(fCurLig, coldeb, fMaxCol);
		DrawArea(fCurLig, coldeb, fMaxCol);
		Window()->Unlock();
	}
	for (c = coldeb; c < coldeb + nbre; c++)
	{
		empty_car(&fMemPages[fCurLig][c]);
	}
}

//------------------------------------------------------------------------

void MinitelView::PositionLC(int32 ligne, int32 colonne)
// affectation position
{
	float	x;
	float	y;

	if (fCursorFlag)
		HideCursor(true);
	fCurLig = ligne;
	fCurCol = colonne;
	x = (fCurCol - 1) * fRapLarg * fCsteTaille;
	y = (kMAX_LIG - fCurLig) * fCsteTaille;
/* recadrage si necessaire */
	NormPos();
/* remise a 0 du delimiteur si 80 col*/
	if (fMaxCol != 40)
		fMemPages[fCurLig][fCurCol].delim = 0;
/* curseur si necessaire */
	if (fCursorFlag)
		HideCursor(false);
}

//------------------------------------------------------------------------

void MinitelView::ScrollPart(int32 lignedeb, int32 lignefin, int32 nbre, int32 sens)
{
	int32	l;
	int32	c;

	if (sens == 1)
	{
		for (l = lignedeb; l <= lignefin; l++)
		{
			if (l - nbre >= 1)
			{
				for (c = kMIN_COL; c <= fMaxCol; c++)
				{
					fMemPages[l - nbre][c] = fMemPages[l][c];
				}
			}
		}
		if (fDraw)
		{
			if (fCursorFlag)
				HideCursor(true);
			Window()->Lock();
			BRect	src, dest;
			src.left = 0;
			src.right = src.left + fRapLarg * fCsteTaille * fMaxCol;
			src.top = fCsteTaille * lignedeb;
			src.bottom = fCsteTaille * (lignefin + 1);
			dest = src;
			dest.top = src.top - fCsteTaille * nbre;
			dest.bottom = src.bottom - fCsteTaille * nbre;
			CopyBits(src, dest);
			if (lignedeb - nbre <= 0)
				DrawRangee0(fOldStatus);
		//	for (l = lignedeb; l <= lignefin; l++)
		//	{
		//		if (l - nbre >= 1)
		//		{
		////		DrawBack(l - nbre, 1, fMaxCol);
		////		DrawText(l - nbre, 1, fMaxCol);
		//			DrawArea(l - nbre, 1, fMaxCol);
		//		}
		//	}
		}
		EffaceLignes(lignefin - nbre + 1, lignefin);
		if (fDraw)
		{
			Window()->Unlock();
			if (fCursorFlag)
				HideCursor(false);
		}
	}
	else
	{
		for (l = lignefin; l >= lignedeb; l--)
		{
			if (l + nbre <= kMAX_LIG)
			{
				for (c = kMIN_COL; c <= fMaxCol; c++)
				{
					fMemPages[l + nbre][c] = fMemPages[l][c];
				}
			}
		}
		if (fDraw)
		{
			if (fCursorFlag)
				HideCursor(true);
			Window()->Lock();
			BRect	src, dest;
			src.left = 0;
			src.right = src.left + fRapLarg * fCsteTaille * fMaxCol;
			src.top = fCsteTaille * lignedeb;
			src.bottom = fCsteTaille * (lignefin + 1);
			dest = src;
			dest.top = src.top + fCsteTaille*nbre;
			dest.bottom = src.bottom + fCsteTaille*nbre;
			CopyBits(src, dest);
		//	for (l = lignefin; l >= lignedeb; l--)
		//	{
		//		if (l + nbre <= kMAX_LIG)
		//		{
		////		DrawBack(l + nbre, 1, fMaxCol);
		////		DrawText(l + nbre, 1, fMaxCol);
		//			DrawArea(l + nbre, 1, fMaxCol);
		//		}
		//	}
		}
		EffaceLignes(lignedeb, lignedeb + nbre - 1);
		if (fDraw)
		{
			Window()->Unlock();
			if (fCursorFlag)
				HideCursor(false);
		}
	}
}

//------------------------------------------------------------------------

void MinitelView::SetAttZone(int32 color, int32 masq_flag, int32 slgn_flag)
// chgt couleur fond, masquage, souligne / zone
{	
	fMemPages[fCurLig][fCurCol].delim = 0;
	DoAttZone(color, masq_flag, slgn_flag);
	fMemPages[fCurLig][fCurCol].delim = 1; /* attribut a valider */
	if (get_haut(fCurPol) == 2)
	{
		fCurLig--;
		fMemPages[fCurLig][fCurCol].delim = 0;
		DoAttZone(color, masq_flag, slgn_flag);
		fMemPages[fCurLig][fCurCol].delim = 1;
		fCurLig++;
	}
}

//------------------------------------------------------------------------

void MinitelView::SetRangee0(int32 status)
{
	if (Window()->Lock())
	{
		if (fDraw)
		{
			if (fCursorFlag)
				HideCursor(true);
			DrawRangee0(status);
			if (fCursorFlag)
				HideCursor(false);
		}
		Window()->Unlock();
	}
}

//------------------------------------------------------------------------

void MinitelView::SetStdColor()
{
	fCouleurs[eNoir].red = fCouleurs[eNoir].green = fCouleurs[eNoir].blue = 0;
	fCouleurs[eRouge].red = 255; fCouleurs[eRouge].green = fCouleurs[eRouge].blue = 0;
	fCouleurs[eVert].green = 255; fCouleurs[eVert].red = fCouleurs[eVert].blue = 0;
	fCouleurs[eJaune].red = fCouleurs[eJaune].green = 255; fCouleurs[eJaune].blue = 0;
	fCouleurs[eBleu].blue = 255; fCouleurs[eBleu].red = fCouleurs[eBleu].green = 0;
	fCouleurs[eMagenta].red = fCouleurs[eMagenta].blue = 255; fCouleurs[eMagenta].green = 0;
	fCouleurs[eCyan].green = fCouleurs[eCyan].blue = 255; fCouleurs[eCyan].red = 0;
	fCouleurs[eBlanc].red = fCouleurs[eBlanc].green = fCouleurs[eBlanc].blue = 255;
	Window()->Lock();
	Invalidate();
	Window()->Unlock();
}

//------------------------------------------------------------------------

void MinitelView::SetStdGrey()
{
	fCouleurs[eNoir].red = fCouleurs[eNoir].green = fCouleurs[eNoir].blue = 0;
	fCouleurs[eBleu].red = fCouleurs[eBleu].green = fCouleurs[eBleu].blue = 42;
	fCouleurs[eRouge].red = fCouleurs[eRouge].green = fCouleurs[eRouge].blue = 84;
	fCouleurs[eMagenta].red = fCouleurs[eMagenta].green = fCouleurs[eMagenta].blue = 127;
	fCouleurs[eVert].red = fCouleurs[eVert].green = fCouleurs[eVert].blue = 170;
	fCouleurs[eCyan].red = fCouleurs[eCyan].green = fCouleurs[eCyan].blue = 212;
	fCouleurs[eJaune].red = fCouleurs[eJaune].green = fCouleurs[eJaune].blue = 232;
	fCouleurs[eBlanc].red = fCouleurs[eBlanc].green = fCouleurs[eBlanc].blue = 255;
	Window()->Lock();
	Invalidate();
	Window()->Unlock();
}

//------------------------------------------------------------------------

void MinitelView::Zoom(float factor)
{
	int32	ncx = (int32)(fCX * factor);
	int32	ncy;

	if (ncx == fCX)
	{
		if (factor > 1.0)
			ncx++;
		else
			ncx--;
	}
	if (ncx < 5)
		ncx = 5;

	if (fMaxCol == 80)
		ncy = (10 * ncx) / 6;
	else
		ncy = (10 * ncx) / 8;
	Window()->Lock();
	SetCellSize(ncx, ncy);
	Window()->Unlock();
}


//========================================================================

void MinitelView::AffiCursor()
{
	struct mem_car	cur_aff;

	if (fCursorState == kCURSOR_PAUSE)
	{
		return;
	}
	cur_aff = fSaveCursor;

	Window()->Lock();
//	affectePolice(save_cursor.pol);
//	cursor_state = cursor_state*-1;
	switch (fCursorState)
	{
		case kCURSOR_OFF:
			if (fMaxCol == 40)
				cur_aff.inv = 1 - fSaveCursor.inv;
			else
				cur_aff.soul = 1 - fSaveCursor.soul;
			fCursorState = kCURSOR_ON;
			break;

		case kCURSOR_ON:
			fCursorState = kCURSOR_OFF;
			break;
	}
	DoAff(cur_aff);
	Window()->Unlock();
}

//------------------------------------------------------------------------

void MinitelView::Couleur(int32 color)
{
	if (fDraw)
		SetHighColor(fCouleurs[color]);
}

//------------------------------------------------------------------------

void MinitelView::DoAff(struct mem_car c)
{
	if ((fCurLig == 0) && (fCurCol == fMaxCol - 1))
		return;
	DoAff(c, fCurLig, fCurCol);
}

//------------------------------------------------------------------------

void MinitelView::DoAff(struct mem_car c, int32 lig, int32 col)
// affichage caractere c.car avec attributs c.pol, c.fnd,...
{
	if (fDraw)
		DrawChar(c, lig, col);
}

//------------------------------------------------------------------------

void MinitelView::DoAttZone(int32 color, int32 masq_flag, int32 slgn_flag)
// aff coul fond, masq, soul / zone
{
	int32	i;
	struct	mem_car caractere;

//	DPRINTF_TV("Att zone lig=%d col=%d col_fnd=%d slgn=%d\n",fCurLig,fCurCol,color, slgn_flag);
	
	if ((fMemPages[fCurLig][fCurCol].col_fnd == color) &&
		((fMemPages[fCurLig][fCurCol].masq == masq_flag) || (masq_flag == -1)) &&
		((fMemPages[fCurLig][fCurCol].soul == slgn_flag)  || (slgn_flag == -1)))
		return;


	for (i = fCurCol; i <= fMaxCol; i = i + get_larg(fCurPol))
	{
		if (fMemPages[fCurLig][i].delim != 0) 
		{
//			DPRINTF_TV("cas att zone arret avant fin (%d) delim = %d\n", i, fMemPages[fCurLig][i].delim);
			break;
		}
		else
		{
			fMemPages[fCurLig][i].col_fnd = color;
			if (masq_flag >= 0)
				fMemPages[fCurLig][i].masq = masq_flag;
			if (slgn_flag >= 0)
				fMemPages[fCurLig][i].soul = slgn_flag;
			if (get_larg(fCurPol) == 2)
			{
				fMemPages[fCurLig][i+1].col_fnd = color;
				if (masq_flag >= 0)
					fMemPages[fCurLig][i+1].masq = masq_flag;
				if (slgn_flag >= 0)
					fMemPages[fCurLig][i+1].soul = slgn_flag;
			}
		}
	}
		
//	DPRINTF_TV("fin zone delim = %d\n", i);

/* retracage fin ligne */	
	if (fDraw)
	{
		Window()->Lock();
		if (fCursorFlag)
			HideCursor(true);
	}
	caractere = fMemPages[fCurLig][fCurCol];
	if (caractere.car == '\0')
		caractere.pol = kG0_NORM;
//	redrawArea(fCurLig, fCurCol, i - 1, caractere);
	DrawArea(fCurLig, fCurCol, i - 1);
	if (fDraw)
	{
		Window()->Unlock();
		if (fCursorFlag)
			HideCursor(false);
	}
}

//------------------------------------------------------------------------

int32 MinitelView::DrawChar(mem_car cc, int32 l, int32 c)
{
	FillCharMask(cc);
	if (cc.pol == kG0_DBLARG)
	{
		DrawBitmap(fCharMaskDW, BPoint((c - 1) * fCX, l * fCY));
		return 2;
	}
	else if (cc.pol == kG0_DBTAIL)
	{
		DrawBitmap(fCharMaskDS, BPoint((c - 1) * fCX, (l - 1) * fCY));
		return 2;
	}
	else if (cc.pol == kG0_DBHAUT)
		DrawBitmap(fCharMaskDH, BPoint((c - 1) * fCX, (l - 1) * fCY));
	else
		DrawBitmap(fCharMask, BPoint((c - 1) * fCX, l * fCY));
	return 1;
}

//------------------------------------------------------------------------

void MinitelView::DrawArea(int32 l, int32 c_min, int32 c_max)
{
	if (fDraw)
	{
		for (int32 c = c_min;  c <= c_max;)
		{
			if (fMemPages[l][c].car == '\0') // null (support dbles)
			{
				c++; // skip (no redrawing of neighbours yet)
			}
			else
			{
				c += DrawChar(fMemPages[l][c], l, c);
			}
		}
	}
}

//------------------------------------------------------------------------

void MinitelView::DrawRangee0(int32 status)
{
	int32 c;

	fOldStatus = status;
	
	if (!fDraw)
	{
		for (c = fMaxCol - 3; c < fMaxCol - 1 ; c++)
			empty_car(&fMemPages[0][c]);
	}
	else
	{
		for (c = fMaxCol - 3; c < fMaxCol - 1 ; c++)
		{
			empty_car(&fMemPages[0][c]);
			DoAff(fMemPages[0][c], 0, c);
		}
	}
	empty_car(&fMemPages[0][fMaxCol]);
	if (fDraw)
		DoAff(fMemPages[0][fMaxCol], 0, fMaxCol);
	empty_car(&fMemPages[0][fMaxCol-1]);
	fMemPages[0][fMaxCol-1].inv = 1;
	fMemPages[0][fMaxCol-1].car = 'F';
	fMemPages[0][fMaxCol-1].delim = 2;
	if (status)
		fMemPages[0][fMaxCol-1].car = 'C';
	if (fDraw)
		DrawArea(0, kMIN_COL, fMaxCol);
//		DoAff(fMemPages[0][fMaxCol-1], 0, fMaxCol-1);
}

//------------------------------------------------------------------------

void MinitelView::FillCharMask(mem_car cc)
{
	int32	i, j, ub;
	int32	code = cc.car;
	uchar	fcol, bcol;
	uchar	*psrc, *srcbase, *pdes, c;
	uchar	f;
	BScreen	screen(B_MAIN_SCREEN_ID);

	if (cc.inv)
	{
		fcol = screen.IndexForColor(fCouleurs[cc.col_fnd]);
		bcol = screen.IndexForColor(fCouleurs[cc.col_txt]);
	}
	else
	{
		fcol = screen.IndexForColor(fCouleurs[cc.col_txt]);
		bcol = screen.IndexForColor(fCouleurs[cc.col_fnd]);
	}

	if ((cc.pol == kG1_NORM) || (cc.pol == kG1_NORM + kDSJT_OFF))
	{
		if (code == 0x5f) code = 0x7f;
		if (code > 64) code -= 32;
		code = 14*16 + code - 32;
	}
	else
	{
		if ((code < 32) || (code == 0x7f))
			code = '_';
		code -= 32;
	}
	int32 lbeg = code/16;
	int32 cbeg = code - lbeg*16;
	srcbase = (uchar *)fCharTable->Bits();
	f = screen.IndexForColor(fCouleurs[eBlanc]);
	switch (cc.pol)
	{
		case kG0_DBTAIL: // double size chars
			pdes = (uchar*)fCharMaskDS->Bits();
			ub = (2 * fCX) % 4;
			if (ub)
				ub = 4 - ub;
			for (i = 0; i < fCY; i++)
			{
				psrc = srcbase + 16 * (lbeg * fCY + i) * fCX + cbeg * fCX;
				for (j = 0; j < fCX; j++)
				{
					c = *psrc++;
					if (c == f)
					{
						*pdes++ = fcol;
						*pdes++ = fcol;
					}
					else
					{
						*pdes++ = bcol;
						*pdes++ = bcol;
					}
				}
				for (j = 0; j < ub; j++)
					pdes++;
			/* repeat upper line */
				for (j = 0; j < fCX; j++)
				{
					*pdes = *(pdes - 2 * fCX - ub);
					pdes++;
					*pdes = *(pdes - 1);
					pdes++;
				}
				for (j = 0; j < ub; j++)
					pdes++;
			}

			if (cc.soul)
			{
				pdes = (uchar *)fCharMaskDS->Bits();
				pdes += (2 * fCX + ub) * (2 * fCY - 1);
				for (j = 0; j < 2 * fCX; j++) *pdes++ = fcol;
			}
			break;

		case kG0_DBHAUT: // double height chars
			pdes = (uchar *)fCharMaskDH->Bits();
			ub = fCX % 4;
			if (ub)
				ub = 4 - ub;
			for (i = 0; i < fCY; i++)
			{
				psrc = srcbase + 16 * (lbeg * fCY + i) * fCX + cbeg * fCX;
				for (j = 0; j < fCX; j++)
				{
					c = *psrc++;
					if (c == f)
						*pdes++ = fcol;
					else
						*pdes++ = bcol;
				}
				for (j = 0; j < ub; j++)
					pdes++;
				/* repeat upper line */
				for (j = 0; j < fCX; j++)
				{
					*pdes = *(pdes - fCX - ub);
					pdes++;
				}
				for (j = 0; j < ub; j++)
					pdes++;
			}
			if (cc.soul)
			{
				pdes = (uchar *)fCharMaskDH->Bits();
				pdes += (fCX + ub) * (2 * fCY - 1);
				for (j = 0; j < fCX; j++)
					*pdes++ = fcol;
			}
			break;

		case kG0_DBLARG: // double width chars
			pdes = (uchar*)fCharMaskDW->Bits();
			ub = (2 * fCX) % 4;
			if (ub)
				ub = 4 - ub;
			for (i = 0; i < fCY; i++)
			{
				psrc = srcbase + 16 * (lbeg * fCY + i) * fCX + cbeg * fCX;
				for (j = 0; j < fCX; j++)
				{
					c = *psrc++;
					if (c == f)
					{
						*pdes++ = fcol;
						*pdes++ = fcol;
					}
					else
					{
						*pdes++ = bcol;
						*pdes++ = bcol;
					}
				}
				for (j = 0; j < ub; j++)
					pdes++;
			}
			if (cc.soul)
			{
				pdes = (uchar *)fCharMaskDW->Bits();
				pdes += (2 * fCX + ub) * (fCY - 1);
				for (j = 0; j < 2*fCX; j++)
					*pdes++ = fcol;
			}
			break;

		default: // simple size chars
			pdes = (uchar *)fCharMask->Bits();
			ub = fCX % 4;
			if (ub)
				ub = 4 - ub;
			for (i = 0; i < fCY; i++)
			{
				psrc = srcbase + 16 * (lbeg * fCY + i) * fCX + cbeg * fCX;
				for (j = 0; j < fCX; j++)
				{
					c = *psrc++;
					if (c == f)
						*pdes++ = fcol;
					else
						*pdes++ = bcol;
				}
				for (j = 0; j < ub; j++)
					pdes++;
			}
			if ((cc.soul) && (cc.pol == kG0_NORM) && (!cc.delim))
			{
				pdes = (uchar*)fCharMask->Bits();
				pdes += (fCX + ub) * (fCY - 1);
				for (j = 0; j < fCX; j++)
					*pdes++ = fcol;
			}
			break;
	}
}

//------------------------------------------------------------------------

void MinitelView::HideCursor(bool hide)
{
	if (hide)
	{
		if (fCursorState == kCURSOR_ON)
		{
			fCursorState = kCURSOR_PAUSE;
			if ((Window()) && (Window()->Lock()))
			{
//				affectePolice(fSaveCursor.pol);
				DoAff(fSaveCursor);
				Window()->Unlock();
			}
		}
		else
			fCursorState = kCURSOR_PAUSE;
	}
	else
	{
		if (fCursorState == kCURSOR_PAUSE)
		{
			fCursorState = kCURSOR_OFF;
			fSaveCursor = fMemPages[fCurLig][fCurCol];
//			affiCursor();
		}
	}
}

//------------------------------------------------------------------------

void MinitelView::NormPos()
{
	if (fCurLig < kMIN_LIG)
		fCurLig = kMIN_LIG;
	if (fCurLig > kMAX_LIG)
		fCurLig = kMAX_LIG;
	if (fCurCol < kMIN_COL)
		fCurCol = kMIN_COL;
	if (fCurCol > fMaxCol)
		fCurCol = fMaxCol;
}

//------------------------------------------------------------------------

void MinitelView::Reset()
{
	fCurLig = kMIN_LIG;
	fCurCol = kMIN_COL;
	EnableCursor(true);
	SetInverse(false);
	fCurTxt = eBlanc;
	fCurPol = kG0_NORM;
	fPageRevele = false;
	fSoulFlag = false;
	
	for (int32 l = 0; l <= kMAX_LIG; l++)
		for (int32 c = kMIN_COL; c <= fMaxCol; c++)
			empty_car(&fMemPages[l][c]);
}

//------------------------------------------------------------------------

void MinitelView::Resize()
{
	BRect	r;
	int32	w;
	int32	h;

	r = Frame();
	w = (int32)(fCsteTaille * (fMaxCol - kMIN_COL + 1) * fRapLarg) - 1;
	h = (int32)(fCsteTaille * (kMAX_LIG - kMIN_LIG + 1)) - 1;
	ResizeTo(w, h);
#ifdef BUILD_APP
	#ifdef USE_KEYBOARD_PANEL
	Window()->ResizeTo(w, h + kKEYBOARD_BAR_HEIGHT);
	#else
	Window()->ResizeTo(w, h);
	#endif
#endif
	Invalidate();
}

//------------------------------------------------------------------------

void MinitelView::RestoreAtt()
// restore att zone si delimiteur ecrase
{
	int32	color;
	int32	masq_flag;
	int32	slgn_flag;
	
//	DPRINTF_TV("Restore\n");
	color = eNoir;
	masq_flag = 0;
	slgn_flag = 0;
	if (fCurCol > 1) 
	{
		color = fMemPages[fCurLig][fCurCol - 1].col_fnd;
		masq_flag = fMemPages[fCurLig][fCurCol - 1].masq;
		slgn_flag = fMemPages[fCurLig][fCurCol - 1].soul;
	}
	DoAttZone(color, masq_flag, slgn_flag);
}

//------------------------------------------------------------------------

void MinitelView::SetCellSize(int32 x, int32 y)
{
	int32		index;
	BBitmap*	bitmap = NULL;
	BFont		font;
	BRect		r;

	bitmap = new BBitmap(BRect(0, 0, kFONT_BITMAP_WIDTH, kFONT_BITMAP_HEIGHT), B_CMAP8, true);
	bitmap->SetBits(kMINITEL_FONT, bitmap->BitsLength(), 0, B_CMAP8);

	fCX = x;
	fCY = y;

	font.SetFamilyAndStyle(kDEFAULT_FONT, "Roman");
	font.SetSize((int)(.8 * min_c(fCX, fCY) + .5));
	font.SetFlags(B_DISABLE_ANTIALIASING);

	if (fCharMask)
		delete fCharMask;
	BRect cBounds(0, 0, fCX - 1, fCY - 1);
	fCharMask = new BBitmap(cBounds, B_COLOR_8_BIT, FALSE);

	if (fCharMaskDW)
		delete fCharMaskDW;
	BRect dBounds(0, 0, 2 * fCX - 1, fCY - 1);
	fCharMaskDW = new BBitmap(dBounds, B_COLOR_8_BIT, FALSE);

	if (fCharMaskDH)
		delete fCharMaskDH;
	dBounds.Set(0, 0, fCX - 1, 2 * fCY - 1);
	fCharMaskDH = new BBitmap(dBounds, B_COLOR_8_BIT, FALSE);

	if (fCharMaskDS)
		delete fCharMaskDS;
	dBounds.Set(0, 0, 2 * fCX - 1, 2 * fCY - 1);
	fCharMaskDS = new BBitmap(dBounds, B_COLOR_8_BIT, FALSE);

	if (fCharTable)
		delete fCharTable;
	BRect tBounds(0, 0, fCX * 16 - 1, fCY * 18 - 1); // 18 lines of 16 chars
	fCharTable = new BBitmap(tBounds, B_COLOR_8_BIT, FALSE);

	BBitmap *tmpB = new BBitmap(tBounds, B_COLOR_8_BIT, TRUE);
	BView *tmpV = new BView(tBounds, NULL, 0, 0);
	tmpB->AddChild(tmpV);
	tmpB->Lock();
	tmpV->SetFont(&font);
	int32 c, i;
	float ox, oy, dx, dy;
	dx = fCX / 10. + 1;
	dy = .8 * fCY - 1;
	for (i = 32, c = 0, ox = oy = 0.; i < 256;) // ascii chars
	{
		tmpV->SetHighColor(fCouleurs[eNoir]);
		cBounds.OffsetTo(ox, oy),
		tmpV->FillRect(cBounds);
		tmpV->SetLowColor(fCouleurs[eNoir]);
		tmpV->SetHighColor(fCouleurs[eBlanc]);
		tmpV->SetPenSize(1);
		//tmpV->SetDrawingMode(B_OP_COPY);
		// following for psuedo ascii
		switch (i)
		{
			//case 0x2f: // slash
			//	tmpV->MovePenTo(ox, oy + fCY - 1);
			//	tmpV->StrokeLine(BPoint(ox + fCX - 1, oy));
			//	break;

			//case 0x5c: // antislash
			//	tmpV->MovePenTo(ox, oy);
			//	tmpV->StrokeLine(BPoint(ox + fCX - 1, oy + fCY - 1));
			//	break;

			//case 0x5f: // underline
			//	tmpV->MovePenTo(ox, oy + fCY - 1);
			//	tmpV->StrokeLine(BPoint(ox + fCX - 1, oy + fCY - 1));
			//	break;

			//case 0x60: // hor midline
			//	tmpV->MovePenTo(ox, oy + fCY / 2 - 1);
			//	tmpV->StrokeLine(BPoint(ox + fCX - 1, oy + fCY / 2 - 1));
			//	break;

			//case 0x7b: // vert left line
			//	tmpV->MovePenTo(ox, oy);
			//	tmpV->StrokeLine(BPoint(ox, oy + fCY - 1));
			//	break;

			//case 0x7c: // vert midline
			//	tmpV->MovePenTo(ox + fCX / 2 - 1, oy);
			//	tmpV->StrokeLine(BPoint(ox + fCX / 2 - 1, oy + fCY - 1));
			//	break;

			//case 0x7d: // vert right line
			//	tmpV->MovePenTo(ox + fCX - 1, oy);
			//	tmpV->StrokeLine(BPoint(ox + fCX - 1, oy + fCY - 1));
			//	break;

			//case 0x7e: // upperline
			//	tmpV->MovePenTo(ox, oy);
			//	tmpV->StrokeLine(BPoint(ox + fCX - 1, oy));
			//	break;

			case 0x88: // à
				index = 0x93;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("à");
				break;

			case 0x8f: // è
				index = 0x97;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("è");
				break;

			case 0x9d: // ù
				index = 0xa0;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ù");
				break;

			case 0x8e: // é
				index = 0x98;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("é");
				break;

			case 0x89: // â
				index = 0x94;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("â");
				break;

			case 0x90: // ê
				index = 0x99;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ê");
				break;

			case 0x94: // î
				index = 0x9b;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("î");
				break;

			case 0x99: // ô
				index = 0x9d;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ô");
				break;

			case 0x9e: // û
				index = 0xa1;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("û");
				break;

			case 0x8a: // ä
				index = 0x95;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ä");
				break;

			case 0x91: // ë
				index = 0x9a;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ë");
				break;

			case 0x95: // ï
				index = 0x9c;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ï");
				break;

			case 0x9a: // ö
				index = 0x9e;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ö");
				break;

			case 0x9f: // ü
				index = 0xa2;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ü");
				break;

			case 0x8d: // ç
				index = 0x96;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ç");
				break;

			case 0xa3: // £ (British Sterling)
				index = 0x81;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("£");
				break;

			case 0xa4: // § (Paragraph)
				index = 0x82;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("§");
				break;

			case 0xa0: // ˆ	(Dagger)
				index = 0x86;	/* probably wrong - rmp */
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ˆ");
				break;

			case 0xa1: // ° (Degree)
				index = 0x84;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("°");
				break;

			case 0xba: // ± (+ over -)
				index = 0x85;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("±");
				break;

			case 0xd6: // ÷ (Divide)
				index = 0x9f;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("÷");
				break;

			case 0xd7: // ¼ (1/4)
				index = 0x88;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("¼");
				break;

			case 0xd8: // ½ (1/2)
				index = 0x89;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("½");
				break;

			case 0xd9: // ¾ (3/4)
				index = 0x8a;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("¾");
				break;

			case 0xce: // Œ (OE)
				index = 0x7f;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("Œ");
				break;

			case 0xcf: // œ (oe)
				index = 0x80;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("œ");
				break;

			case 0xa7: // ß (beta)
				index = 0x92;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawString("ß");
				break;

			default:
				index = i;
				//tmpV->MovePenTo(ox + dx, oy + dy);
				//tmpV->DrawChar(i);
				break;
		}

		if (index != ' ')
		{
			r.left = ((index - '!') % 26) * kFONT_CELL_WIDTH;
			r.right = r.left + kFONT_CELL_WIDTH - 1;
			r.top = ((index - '!') / 26) * kFONT_CELL_HEIGHT;
			r.bottom = r.top + kFONT_CELL_HEIGHT - 1;

			tmpV->DrawBitmap(bitmap, r, cBounds);
		}
		tmpV->Sync();
		i++;
		c++;
		if (c < 16)
			ox += fCX;
		else
		{
			oy += fCY;
			c = 0;
			ox = 0.;
		}
	}
	
	dx = fCX / 2;
	dy = fCY / 3;
	BRect c1(0, 0, dx - 1, dy - 1); // upper left cell
	BRect c2(dx, 0, fCX - 1, dy - 1); // upper right cell
	BRect c3(0, dy, dx - 1, 2 * dy - 1); // mid left cell
	BRect c4(dx, dy, fCX - 1, 2 * dy - 1); // mid right cell
	BRect c5(0, 2*dy, dx - 1, fCY - 1); // lower left cell
	BRect c6(dx, 2*dy, fCX - 1, fCY - 1); // lower right cell
	uchar m;
	for (i = 32, c = 0, ox = 0., oy = 14 * fCY; i < 64;)
	{ // mosaics 1st part
		tmpV->SetHighColor(fCouleurs[eNoir]);
		cBounds.OffsetTo(ox, oy),
		tmpV->FillRect(cBounds);
		tmpV->SetHighColor(fCouleurs[eBlanc]);
		m = i - 0x20;
		if (m & 0x01)
		{
			c1.OffsetTo(ox, oy),
			tmpV->FillRect(c1);
		}
		if (m & 0x02)
		{
			c2.OffsetTo(ox + dx, oy),
			tmpV->FillRect(c2);
		}
		if (m & 0x04)
		{
			c3.OffsetTo(ox, oy + dy),
			tmpV->FillRect(c3);
		}
		if (m & 0x08)
		{
			c4.OffsetTo(ox + dx, oy + dy),
			tmpV->FillRect(c4);
		}
		if (m & 0x10)
		{
			c5.OffsetTo(ox, oy + 2 * dy),
			tmpV->FillRect(c5);
		}
		i++;
		c++;
		if (c < 16)
			ox += fCX;
		else
		{
			oy += fCY;
			c = 0;
			ox = 0;
		}
		tmpV->Sync();
	}
	for (i = 96, c = 0, ox = 0., oy = 16 * fCY; i < 128;)
	{ // mosaics 2nd part
		tmpV->SetHighColor(fCouleurs[eNoir]);
		cBounds.OffsetTo(ox, oy),
		tmpV->FillRect(cBounds);
		tmpV->SetHighColor(fCouleurs[eBlanc]);
		m = i - 0x20;
		if (m & 0x01)
		{
			c1.OffsetTo(ox, oy),
			tmpV->FillRect(c1);
		}
		if (m & 0x02)
		{
			c2.OffsetTo(ox + dx, oy),
			tmpV->FillRect(c2);
		}
		if (m & 0x04)
		{
			c3.OffsetTo(ox, oy + dy),
			tmpV->FillRect(c3);
		}
		if (m & 0x08)
		{
			c4.OffsetTo(ox + dx, oy + dy),
			tmpV->FillRect(c4);
		}
		if (m & 0x10)
		{
			c5.OffsetTo(ox, oy + 2 * dy),
			tmpV->FillRect(c5);
		}
		c6.OffsetTo(ox + dx, oy + 2 * dy),
		tmpV->FillRect(c6);
		i++;
		c++;
		if (c < 16)
			ox += fCX;
		else
		{
			oy += fCY;
			c = 0;
			ox = 0;
		}
		tmpV->Sync();
	}
	tmpB->Unlock();
//	fCharTable->SetBits(tmpB->Bits(), tmpB->BitsLength(), 0, B_COLOR_8_BIT);
	uchar	*psrc;
	uchar	*pdes;
	int32	len;

	psrc = (uchar*)tmpB->Bits();
	len = tmpB->BitsLength();
	pdes = (uchar*)fCharTable->Bits();
	for (i = 0; i < len; i++) {
//		fprintf(stderr,"%02x\n", *psrc);
		*pdes++ = *psrc++;
	}
	delete tmpB;

	SetFont(&font);
	fCsteTaille = y;
	fRapLarg = ((float)x) / ((float)y);
	Resize();
	delete bitmap;
}

//------------------------------------------------------------------------

status_t ConnectThread(void* data)
{
	connect_data*	c = (connect_data*)data;

	c->modem->Connect(c->number);
	free(c->number);
	delete c;
	return B_NO_ERROR;
}


//========================================================================

#ifdef USE_KEYBOARD_PANEL

#include <MenuItem.h>


#define kBUTTON_WIDTH		 60
#define kCONNECT_LABEL		"Connexion" //"Connexion/Fin"
#define kSUMMARY_LABEL		"Sommaire"
#define kGUIDE_LABEL		"Guide"
#define kREPEAT_LABEL		"Répétition"
#define kCORRECT_LABEL		"Correction"
#define kBACK_LABEL			"Retour"
#define kENTER_LABEL		"Envoi"
#define kCANCEL_LABEL		"Annulation"
#define kNEXT_LABEL			"Suite"
#define kPHONE_TEXT			"Composer"	//"Dial"
#define kADD_TEXT			"Ajouter"	//"Add"
#define kREMOVE_TEXT		"Effacer"	//"Remove"
#define kMINITEL_DIRECTORY	"MinitelDirectory"
#define kNO_MENU_ITEMS		"Vide"		//"None"


enum	COLORS
{
	eDefault = 0,
	eRed,
	eGreen,
	eBlue
};


struct	button_data
{
	char	label[256];
	BPoint	p;
	int32	m;
	int32	c;
};


//========================================================================

KeyboardPanel::KeyboardPanel(BRect rect, MinitelView* view)
	: BBox				(rect,
						 "keyboard",
						 B_FOLLOW_BOTTOM,
						 B_WILL_DRAW),
	fChanged			(false),
	fMinitelDirectory	(NULL),
	fView				(view)
{
	BDirectory	dir;
	BEntry		entry;
	BPath		path;

	SetViewColor(222, 222, 222, 255);

	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	dir.SetTo(path.Path());
	if (dir.FindEntry(kMINITEL_DIRECTORY, &entry) == B_NO_ERROR)
	{
		fMinitelDirectory = new BFile(&entry, O_RDWR);
		if (fMinitelDirectory->InitCheck() != B_NO_ERROR)
		{
			delete fMinitelDirectory;
			fMinitelDirectory = NULL;
		}
	}
	else
	{
		fMinitelDirectory = new BFile();
		if (dir.CreateFile(kMINITEL_DIRECTORY, fMinitelDirectory) != B_NO_ERROR)
		{
			delete fMinitelDirectory;
			fMinitelDirectory = NULL;
		}
		else
		{
			BNodeInfo	info;

			info.SetTo(fMinitelDirectory);
			info.SetType("text/plain");
		}
	}
}

//------------------------------------------------------------------------

KeyboardPanel::~KeyboardPanel()
{
	if (fChanged)
		SaveMenu();
}

//------------------------------------------------------------------------

void KeyboardPanel::AttachedToWindow()
{
	float		gap = (Bounds().Width() - (kBUTTON_WIDTH * 6)) / 7;
	BButton*	button = NULL;
	BFont		font(be_plain_font);
	BRect		r = Bounds();
	button_data	buttons[] = {{kCONNECT_LABEL, BPoint(0, 0), eButtonConnect, eRed},
							 {kSUMMARY_LABEL, BPoint(2, 0), eButtonSummary, eDefault},
							 {kCANCEL_LABEL,  BPoint(3, 0), eButtonCancel, eDefault},
							 {kBACK_LABEL,    BPoint(4, 0), eButtonBack, eDefault},
							 {kREPEAT_LABEL,  BPoint(5, 0), eButtonRepeat, eDefault},
							 {kGUIDE_LABEL,   BPoint(2, 1), eButtonGuide, eDefault},
							 {kCORRECT_LABEL, BPoint(3, 1), eButtonCorrect, eDefault},
							 {kNEXT_LABEL,    BPoint(4, 1), eButtonNext, eDefault},
							 {kENTER_LABEL,   BPoint(5, 1), eButtonEnter, eGreen}};

	for (uint32 i = 0; i < sizeof(buttons) / sizeof(button_data); i++)
	{
		r.left = gap + (buttons[i].p.x * (gap + kBUTTON_WIDTH));
		r.right = r.left + kBUTTON_WIDTH;
		(button) ? r.top = 3 + (buttons[i].p.y * (3 + button->Bounds().Height()))
				 : r.top = 3;
		r.bottom = 0;
		AddChild(button = new BButton(r, buttons[i].label, buttons[i].label, new BMessage(buttons[i].m)));
		switch (buttons[i].c)
		{
			case eDefault:
				break;
			case eRed:
				button->SetViewColor(255, 0, 0, 255);
				break;
			case eGreen:
				button->SetViewColor(0, 255, 0, 255);
				break;
			case eBlue:
				button->SetViewColor(0, 0, 255, 255);
				break;
		}
		button->SetTarget(this);
	}

	/* phone menu */
	fMenu = new BPopUpMenu("");
	r.Set(0, 0, 0, 0);
	AddChild(fMenuField = new BMenuField(r, "", "", fMenu));
	fMenuField->SetDivider(0);
	r = fMenuField->MenuBar()->Frame();
	fMenuField->MoveTo(2 * (gap + kBUTTON_WIDTH) - r.Width() - gap,
					   3 + button->Bounds().Height() + ((button->Bounds().Height() - r.Height()) / 2));
	fMenu->SetRadioMode(false);

	/* phone number */
	r = fMenuField->Frame();
	r.right = r.left - 1;
	r.left = gap;
	AddChild(fPhoneText = new BTextControl(r, "phone", kPHONE_TEXT, "3611", new BMessage(eTextPhone)));
	r = fPhoneText->Frame();
	fPhoneText->MoveTo(r.left,
					   r.top + ((fMenuField->Frame().Height() - r.Height()) / 2) + 1);
	fPhoneText->SetDivider(font.StringWidth(kPHONE_TEXT) + 3);
	fPhoneText->SetTarget(this);
	fPhoneText->SetModificationMessage(new BMessage(ePhoneModified));

	BuildMenu();
}

//------------------------------------------------------------------------

void KeyboardPanel::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case eMenuNumber:
			{
				int32		index;
				BMenuItem	*item;

				msg->FindInt32("index", &index);
				item = fMenu->ItemAt(index);
				fPhoneText->SetText(item->Label());
				fPhoneText->TextView()->SelectAll();
				CheckMenus();
			}
			break;

		case eMenuAdd:
			{
				BMenuItem*	item;

				item = fMenu->ItemAt(0);
				if ((item) && (item->IsEnabled() == false))
					fMenu->RemoveItem(item);
				fMenu->AddItem(item = new BMenuItem(fPhoneText->Text(), new BMessage(eMenuNumber)), 0);
				item->SetTarget(this);
				fChanged = true;
				SaveMenu();
				CheckMenus();
			}
			break;

		case eMenuRemove:
			{
				int32	index;

				index = FindMenuItem(fPhoneText->Text());
				if (index >= 0)
				{
					BMenuItem*	item;

					fMenu->RemoveItem(index);
					item = fMenu->ItemAt(0);
					if ((item) && (strlen(item->Label()) == 0))
					{
						fMenu->AddItem(item = new BMenuItem(kNO_MENU_ITEMS, new BMessage(eMenuNumber)), 0);
						item->SetEnabled(false);
					}
					fChanged = true;
				}
				SaveMenu();
				CheckMenus();
			}
			break;

		case ePhoneModified:
			CheckMenus();
			break;

		case eButtonConnect:
		case eTextPhone:
			{
				const char*	text = fPhoneText->Text();
				msg->AddString("number", text);
			}
			// fall through

		default:
			Window()->PostMessage(msg, fView);
	}
	if ((msg->what >= eButtonConnect) && (msg->what <= eButtonNext))
		fView->MakeFocus(true);
}

//------------------------------------------------------------------------

void KeyboardPanel::BuildMenu()
{
	int32		count = 0;
	BMenuItem*	item;

	if (fMinitelDirectory)
	{
		char*	buffer;
		char*	start;
		off_t	index = 0;
		off_t	size;

		fMinitelDirectory->GetSize(&size);
		buffer = (char*)malloc(size);
		if (buffer)
		{
			fMinitelDirectory->Read(buffer, size);
			start = buffer;
			while (index < size)
			{
				if (buffer[index] == '\n')
				{
					buffer[index] = 0;
					fMenu->AddItem(item = new BMenuItem(start, new BMessage(eMenuNumber)));
					item->SetTarget(this);
					if (!count)
					{
						fPhoneText->SetText(start);
						fPhoneText->TextView()->SelectAll();
					}
					start = &buffer[index + 1];
					count++;
				}
				index++;
			}
			free(buffer);
		}
	}

	if (!count)
	{
		fMenu->AddItem(item = new BMenuItem(kNO_MENU_ITEMS, new BMessage(eMenuNumber)));
		item->SetEnabled(false);
	}

	fMenu->AddItem(new BSeparatorItem());
	fMenu->AddItem(fSave = new BMenuItem(kADD_TEXT, new BMessage(eMenuAdd)));
	fSave->SetTarget(this);
	fMenu->AddItem(fDelete = new BMenuItem(kREMOVE_TEXT, new BMessage(eMenuRemove)));
	fDelete->SetTarget(this);
	CheckMenus();
}

//------------------------------------------------------------------------

void KeyboardPanel::CheckMenus()
{
	if (strlen(fPhoneText->Text()))
	{
		if (FindMenuItem(fPhoneText->Text()) >= 0)
		{
			fSave->SetEnabled(false);
			fDelete->SetEnabled(true);
		}
		else
		{
			fSave->SetEnabled(true);
			fDelete->SetEnabled(false);
		}
	}
	else
	{
		fSave->SetEnabled(false);
		fDelete->SetEnabled(false);
	}
}

//------------------------------------------------------------------------

int32 KeyboardPanel::FindMenuItem(const char* text)
{
	int32		index = 0;
	BMenuItem*	item;

	while ((item = fMenu->ItemAt(index)) && (strlen(item->Label())))
	{
		if (strcmp(text, item->Label()) == 0)
			return index;
		index++;
	}
	return -1;
}

//------------------------------------------------------------------------

void KeyboardPanel::SaveMenu()
{
	int32		index = 0;
	BMenuItem*	item;

	fMinitelDirectory->SetSize(0);
	fMinitelDirectory->Seek(0, SEEK_SET);
	while ((item = fMenu->ItemAt(index++)) && (strlen(item->Label())))
	{
		if (strcmp(item->Label(), kNO_MENU_ITEMS) != 0)
		{
			fMinitelDirectory->Write(item->Label(), strlen(item->Label()));
			fMinitelDirectory->Write("\n", 1);
		}
	}
	fChanged = false;
}

#endif	// USE_KEYBOARD_PANEL


//========================================================================

#ifdef USE_PREFERENCE_PANEL

#ifndef kBUTTON_WIDTH
#define kBUTTON_WIDTH		 75
#endif

#define kBUTTON_GAP			  6
#define kWINDOW_HEIGHT		150
#define kWINDOW_WIDTH		200
#define kCOLOR_TEXT			"Color"
#define kGRAY_TEXT			"Gray"
#define kSCREEN_TEXT		"Screen"
#define kZOOM_DOWN_TEXT		"Zoom Down"
#define kZOOM_UP_TEXT		"Zoom Up"

enum	PREF_MESSAGES
{
	eButtonZoomUp = 0,
	eButtonZoomDown,
	eRadioColor,
	eRadioGray
};


//========================================================================

PreferenceWindow::PreferenceWindow(MinitelView* opera)
	: BWindow		(BRect(0, 0, kWINDOW_WIDTH, kWINDOW_HEIGHT),
					 "Preferences",
					 B_FLOATING_WINDOW_LOOK,
					 B_FLOATING_SUBSET_WINDOW_FEEL,
					 B_NOT_CLOSABLE
					 | B_NOT_ZOOMABLE
					 | B_NOT_MINIMIZABLE
					 | B_NOT_RESIZABLE
					 | B_WILL_ACCEPT_FIRST_CLICK),
	  fMinitel		(opera)
{
	BBox*			box;
	BBox*			screen;
	BButton*		button = NULL;
	BRadioButton*	radio;
	BRect			r = Bounds();

	AddToSubset(opera->Window());
	r.right++;
	r.bottom++;
	AddChild(box = new BBox(r, "preferences", B_FOLLOW_ALL));

	r.Set(kBUTTON_GAP, kBUTTON_GAP, kWINDOW_WIDTH - kBUTTON_GAP, 70);
	box->AddChild(screen = new BBox(r, "screen", B_FOLLOW_NONE));
	screen->SetLabel(kSCREEN_TEXT);

	r.Set(kBUTTON_GAP * 4, kBUTTON_GAP * 3, kWINDOW_WIDTH - (kBUTTON_GAP * 8), 0);
	screen->AddChild(radio = new BRadioButton(r, "color", kCOLOR_TEXT, new BMessage(eRadioColor)));
	radio->SetValue(1);

	r.OffsetBy(0, radio->Frame().Height() + 0);
	screen->AddChild(radio = new BRadioButton(r, "gray", kGRAY_TEXT, new BMessage(eRadioGray)));

	r.left = (kWINDOW_WIDTH - (kBUTTON_WIDTH * 2)) / 3;
	r.right = r.left + kBUTTON_WIDTH;
	r.top = screen->Frame().bottom + kBUTTON_GAP;
	box->AddChild(button = new BButton(r, "zoom_up", kZOOM_UP_TEXT, new BMessage(eButtonZoomUp)));

	r.right = kWINDOW_WIDTH - r.left;
	r.left = r.right - kBUTTON_WIDTH;
	box->AddChild(button = new BButton(r, "zoom_down", kZOOM_DOWN_TEXT, new BMessage(eButtonZoomDown)));

	/* adjust window size */
	r = button->Frame();
	ResizeTo(kWINDOW_WIDTH, r.bottom + kBUTTON_GAP);

	/* position window */
	r = fMinitel->Frame();
	fMinitel->ConvertToScreen(&r);
	MoveTo(r.right + 11, r.top + (r.Height() / 2 - 2));

	Show();
	Activate(false);
}

//------------------------------------------------------------------------

void PreferenceWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case eButtonZoomUp:
			fMinitel->Zoom(1.2);
			break;

		case eButtonZoomDown:
			fMinitel->Zoom(1.0 / 1.2);
			break;

		case eRadioColor:
			fMinitel->SetStdColor();
			break;

		case eRadioGray:
			fMinitel->SetStdGrey();
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}
#endif
