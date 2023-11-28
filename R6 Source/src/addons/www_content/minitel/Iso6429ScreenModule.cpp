/*************************************************************************
/
/	Iso6429ScreenModule.cpp
/
/	Modified by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#include "Iso6429ScreenModule.h"
#include "MinitelView.h"
#include "Protocole.h"


//========================================================================

static int32 get_csi_param(int32 *arg, int32 count, int32 num, int32 defaut)
{
	int32	resu = defaut;
	int32	i;
	int32	n = 1;
	int32	c;
	
/* trouver debut ss-chaine */
	for (i = 0; i < count; i++)
	{
		if (n == num)
			break;
		if (arg[i] == 0x3b)
			n++;
	}
/* lire -> fin sschaine */
	if (n == num)
	{
		c = 0;
		for ( ; i < count; i++)
		{
			if (arg[i] >= 0x3b)
				break;
			c = c * 10 + (arg[i] - 0x30);
			resu = c;
		}
	}
	return resu;
}


//========================================================================

Iso6429ScreenModule::Iso6429ScreenModule(Protocole* protocole, MinitelView* view)
	: VirtScreenModule	(),
	fAffiChageEcran		(view),
	fProtocole			(protocole),
	fColonne			(1),
	fFndCol				(eNoir),
	fInvFlag			(0),
	fJeu				(0),
	fLigne				(1),
	fRouleauFlag		(1),
	fSauveColonne		(1),
	fSauveFndCol		(eNoir),
	fSauveInvFlag		(0),
	fSauveJeu			(0),
	fSauveLigne			(1),
	fSauveSoulFlag		(0),
	fSauveTxtCol		(eCyan),
	fSoulFlag			(false),
	fTxtCol				(eCyan)
{
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::OutScr(unsigned char s)
/* interface ecran */
{
	if (fLigne == 0)
	{
		if (s == 0x0a)
		{
			fLigne = fY;
			fAffiChageEcran->PositionLC(fLigne, fColonne);
			fAffiChageEcran->SetTxtColor(fTxtCol);
			fAffiChageEcran->SetFndColor(fFndCol);
			if (fInvFlag)
				fAffiChageEcran->SetInverse(true);
			else
				fAffiChageEcran->SetInverse(false);
			if (fSoulFlag)
				fAffiChageEcran->SetSouligne(true);
			else
				fAffiChageEcran->SetSouligne(false);
			fAffiChageEcran->EnableCursor(true);
		}
	}
	
	if (fSpe != 0)
		DoSpe(s);
	else
	{
		if ((s >= 0x20) && (s <= 0x7f))
		{
			AffEcran(s);
		}
		else /* controles */
		{
			switch (s)
			{
				case 0x0:	/* NUL = bourrage */
				case 0x1:	/* ??? */
				case 0x4:	/* ??? */
					break;
						
				case 0x7 :	/* NXBeep(); */
					break; 	/* BEL = cloche */
				
				case 0x8: 	/* BS = gche */
					fColonne--;
					if (fColonne < 1)
						fColonne = 1;
					fAffiChageEcran->PositionLC(fLigne, fColonne);
					break;
						
				case 0x9: 	/* HT = dte */
					fColonne = fColonne + 8;
					if (fColonne > 80)
						fColonne = 80;
					fAffiChageEcran->PositionLC(fLigne, fColonne);
					break;
						
				case 0xB:	/* VT = bas */
				case 0xC:	/* FF = bas */
				case 0xA:	/* LF = bas */
					fLigne++;
					if (fLigne > 24)
						Lig25();
					else
						fAffiChageEcran->PositionLC(fLigne, fColonne);
					break;
						
				case 0xD:	/* CR */
					fColonne = 1;
					fAffiChageEcran->PositionLC(fLigne, fColonne);
					break;
						
				case 0xE:	/* SO = jeu francais */
					fJeu = 1;
					fProtocole->JeuFrancais();
					break;
						
				case 0xF:	/* SI = jeu americain */
					fJeu = 0;
					fProtocole->JeuAmericain();
					break;

				case 0x11:	/* Xon = reprise HC */
					break;

				case 0x13:	/* Xoff = arret HC */
							/* dans notre cas Sep = separateur */
					fSpe = kIGNORE;
					break;	/* ignore suivant */

				case 0x1A:	/* SUB = pave plein */
				case 0x18:	/* CAN = pave plein */
					DoCan();
					break;
						
				case 0x1B:	/* ESC */
					fSpe = kESCAPE;
					break;

				case 0x1F:	/* US = position y x */
					fSpe = kPOSIT_1;
					break;

				default:
					// DPRINTF_EI("CTL Ox%x non supporte\n",(unsigned int)s);
					break;
			} 
		}
	}
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::Reset()
{
	fAffiChageEcran->Init80Col();
	fAffiChageEcran->Efface();
	fAffiChageEcran->EffaceLignes(0, 0);
	fAffiChageEcran->EnableCursor(true);
	fLigne = 1;
	fColonne = 1;
	fAffiChageEcran->PositionLC(fLigne, fColonne);
	fTxtCol = eCyan;
	fFndCol = eNoir;
	fAffiChageEcran->SetTxtColor(fTxtCol);
	fAffiChageEcran->SetFndColor(fFndCol);
	fJeu = 0;
	fRouleauFlag = 1;
	fInvFlag = 0;
	fAffiChageEcran->SetInverse(false);
	fSoulFlag = 0;
	fAffiChageEcran->SetSouligne(false);
	fSauveLigne = 1;
	fSauveColonne = 1;
	fSauveTxtCol = eCyan;
	fSauveFndCol = eNoir;
	fSauveJeu = 0;
	fSauveInvFlag = 0;
	fSauveSoulFlag = 0;
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::SetConnectStat(int32 status)
{
	fAffiChageEcran->SetRangee0(status);
}


//========================================================================

void Iso6429ScreenModule::AffEcran(unsigned char c)
/* affiche caractere */
{
	if (fJeu)
		JeuG1(c);
	else
		JeuG0(c);
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::DoCan()
/* affichage pave plein */
{
	JeuG0((unsigned char)0x7f);
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::DoCsi(unsigned char s)
{	
	fSpe = 0;
	if (fLigne == 0)
		return;

	switch (s)
	{
		case 'A': /* dep vers le haut n fois */
			fLigne = fLigne - get_csi_param(fCsiArg, fCsiCount, 1, 1);;
			if (fLigne < 1)
				fLigne = 1;
			fAffiChageEcran->PositionLC(fLigne, fColonne);    
			break;

		case 'B': /* dep vers le bas n fois */
			fLigne = fLigne + get_csi_param(fCsiArg, fCsiCount, 1, 1);;
			if (fLigne > 24)
				fLigne = 24;
			fAffiChageEcran->PositionLC(fLigne, fColonne);    
			break;

		case 'C': /* dep vers la droite n fois */
			fColonne = fColonne + get_csi_param(fCsiArg, fCsiCount, 1, 1);;
			if (fColonne > 80)
				fColonne = 80;
			fAffiChageEcran->PositionLC(fLigne, fColonne);    
			break;

		case 'D': /* dep vers la gauche n fois */
			fColonne = fColonne - get_csi_param(fCsiArg, fCsiCount, 1, 1);;
			if (fColonne < 1)
				fColonne = 1;
			fAffiChageEcran->PositionLC(fLigne, fColonne);    
			break;

		case 'H': /* pos curseur */
			fY = get_csi_param(fCsiArg, fCsiCount, 1, 1);
			fX= get_csi_param(fCsiArg, fCsiCount, 2, 1);
			if ((fY >= 1) && (fY <= 24) && (fX >= 1) && (fX <= 80))
			{
				fLigne = fY;
				fColonne = fX;
				fAffiChageEcran->PositionLC(fLigne, fColonne);
			}
			break;

		case 'J':
			switch (fCsiArg[0] - 0x30)
			{
				case 0:
					fAffiChageEcran->PositionLC(fLigne, fColonne);
					fAffiChageEcran->EffaceFinEcran();
					break; /* del -> fin ecran */

				case 1:
					fAffiChageEcran->PositionLC(fLigne, fColonne);
					fAffiChageEcran->EffaceDebEcran();
					break; /* del -> haut ecran */

				case 2:
					fAffiChageEcran->Efface();
					fAffiChageEcran->PositionLC(fLigne, fColonne);
					break; /* cls + reste pos */

				default:
					break;
			}
			break;
				
		case 'K':
			switch(fCsiArg[0] - 0x30)
			{
				case 0:
					fAffiChageEcran->PositionLC(fLigne, fColonne);
					fAffiChageEcran->EffaceFinLigne();
					break; /* del -> fin ligne */

				case 1:
					fAffiChageEcran->PositionLC(fLigne, fColonne);
					fAffiChageEcran->EffaceDebLigne();
					break; /* del -> debut ligne */

				case 2:
					fAffiChageEcran->PositionLC(fLigne, fColonne);
					fAffiChageEcran->EffaceDebLigne();
					fAffiChageEcran->EffaceFinLigne();
					break; /* efface ligne */

				default:
					break;
			}
			break;

		case 'L':
			fAffiChageEcran->ScrollPart(fLigne, 24,
			get_csi_param(fCsiArg, fCsiCount, 1, 0), -1);
			break; /* insertion count rangees depuis ligne courante */

		case 'M':
			fAffiChageEcran->ScrollPart(fLigne + get_csi_param(fCsiArg, fCsiCount, 1, 0), 24,
			get_csi_param(fCsiArg, fCsiCount, 1, 0), 1);
			break; /* suppression count rangees depuis ligne courante */
				
		case 'P':
			fAffiChageEcran->DelCar(fColonne, get_csi_param(fCsiArg, fCsiCount, 1, 0));
			break;
				
		case '@':
			fAffiChageEcran->InsCar(fColonne, get_csi_param(fCsiArg, fCsiCount, 1, 0));
			break;

		case 'm':
			switch (get_csi_param(fCsiArg, fCsiCount, 1, 0)) /* chgt attributs car */
			{
				case 0:
					fTxtCol = eCyan; /* aucun attribut */
					fAffiChageEcran->SetTxtColor(fTxtCol);
					fInvFlag = 0;
					fAffiChageEcran->SetInverse(false);
					fSoulFlag = 0;
					fAffiChageEcran->SetSouligne(false);
					break;
						
				case 1:
					fTxtCol = eBlanc; /* surbrillance */			
					fAffiChageEcran->SetTxtColor(fTxtCol);
					break;
						
				case 22:
					fTxtCol = eCyan; /* intensite normale */			
					fAffiChageEcran->SetTxtColor(fTxtCol);
					break;
						
				case 7:
					fInvFlag = 1; /* inversion */
					fAffiChageEcran->SetInverse(true);
					break;
						
				case 27:
					fInvFlag = 0; /* non inversion */
					fAffiChageEcran->SetInverse(false);
					break;
						
				case 4:
					fSoulFlag = 1; /* souligne */
					fAffiChageEcran->SetSouligne(true);
					break;

				case 24:
					fSoulFlag = 0; /* non souligne */
					fAffiChageEcran->SetSouligne(false);
					break;

				default:
				case 5:		/* clignotant */
				case 25:	/* non clignotant */
					break;	/* non supporte */
			}
			break;

		case 0x7b:
			if ((fCsiArg[0] == '?') && (fCsiCount == 1))
				fProtocole->RetourFrom80Col();
			break;

/* !!! les codes suivants ne sont pas (encore) traites !!! */
		case 'h':
			/* insert on si fCsiArg[0] = '4' et fCsiCount = 1 */
			/* blocage clavier si fCsiArg[0] = '2' et fCsiCount = 1 */
			break;

		case 'i':
			/* impression ecran si fCsiCount = 0 */
			break;

		case 'l':
			/* insert off si fCsiArg[0] = '4' et fCsiCount = 1 */
			/* deblocage clavier si fCsiArg[0] = '2' et fCsiCount = 1 */
			break;

		default:
			// DPRINTF_EI("CSI Ox%x non supporte\n",(unsigned int)s);
			break;
	}
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::DoEsc(unsigned char s)
{
	int32 i;

	switch(s)
	{
		case 0x18:
			break; /* CAN annule l'ESC */

		case 0x37:
			fSauveLigne = fLigne;
			fSauveColonne = fColonne;
			fSauveTxtCol = fTxtCol;
			fSauveFndCol = fFndCol;
			fSauveInvFlag = fInvFlag;
			fSauveSoulFlag = fSoulFlag;
			fSauveJeu = fJeu;
			break; /* sauve contexte */

		case 0x38:
			fLigne = fSauveLigne;
			fColonne = fSauveColonne;
			fAffiChageEcran->PositionLC(fLigne, fColonne);
			fTxtCol = fSauveTxtCol;
			fFndCol = fSauveFndCol;
			fAffiChageEcran->SetTxtColor(fTxtCol);
			fAffiChageEcran->SetFndColor(fFndCol);
			fInvFlag = fSauveInvFlag;
			if (fInvFlag)
				fAffiChageEcran->SetInverse(true);
			else
				fAffiChageEcran->SetInverse(false);
			fSoulFlag = fSauveSoulFlag;
			if (fSoulFlag)
				fAffiChageEcran->SetSouligne(true);
			else
				fAffiChageEcran->SetSouligne(false);
			fJeu = fSauveJeu;
			break; /* restore contexte */

		case 0x44:	/* IND = LF */
			OutScr(0x0A);
			break;

		case 0x45:	/* NEL = CR LF */
			OutScr(0x0d);
			OutScr(0x0a);
			break;

		case 0x4d:	/* RI */
			fLigne--;
			if (fLigne < 1)
				Lig00();
			else
				fAffiChageEcran->PositionLC(fLigne, fColonne);
			break;

		case 0x5B:	/* CSI = ISO 6429 */
			fCsiCount = 0;
			for (i = 0; i < kMAX_CSI_COUNT; i++)
				fCsiArg[i] = 0;
			fSpe = kESC_CSI;
			break; /* CSI */
				
		case 0x63:	/* reset */
			Reset();
			break;

		default:
			// DPRINTF_EI("ESC Ox%x non supporte\n",(unsigned int)s);
			if (s < 0x20)
				OutScr(s); /* essai resynchro */
			break;
	}
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::DoPos()
{
	if (fY == 0x40)
	{
		fY = fLigne;
		fLigne = 0;
	}
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::DoSpe(unsigned char s)
/* caracteres speciaux a argument(s) */
{
	switch (fSpe)
	{
		case kESCAPE:
			fSpe = 0;
			DoEsc(s);
			break;
					
		case kPOSIT_1:
			if ((s >= 0x30) && (s <= 0x7f)) 
			{
				fY = s;
				fSpe = kPOSIT_2;
			}
			else
			{
				fSpe = 0;
				OutScr(s);
				return;
			}
			break;			

		case kPOSIT_2:
			if ((s >= 0x30) && (s <= 0x7f)) 
			{
				fX = s;
				DoPos();
				fSpe = 0;
			}
			else
			{
				fSpe = 0;
				OutScr(s);
				return;
			}
			break;			
	
		case kESC_CSI:
			if ((s >= 0x30) && (s <= 0x3f))
			{
				if (fCsiCount < kMAX_CSI_COUNT) 
				{
					fCsiArg[fCsiCount] = s;
					fCsiCount++;
				}
				else
				{
					// DPRINTF_EI("CSI ERROR COUNT\n");
					DoCsi(0);
				}
			}
			else
				DoCsi(s);
			break;

		case kIGNORE:
			fSpe = 0; // DPRINTF_EI(">>> 0x%x \n",(unsigned int)s);
			break; /* ignore */
	}
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::JeuG0(unsigned char c)
/* jeu americain */
{
	char mapped;
	
	switch (c) /* remapping */
	{
		case 0x5e:
			mapped = '^';
			break; /* arrow up */

		default:
			mapped = c;
			break;
	}
	
	fAffiChageEcran->AfficheBrut((unsigned char)mapped);
/* variables fColonne & fLigne */
	fColonne++;
	if (fColonne > 80)
		fColonne = 80;
	else
		fAffiChageEcran->PositionLC(fLigne, fColonne);
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::JeuG1(unsigned char c)
/* jeu francais */
{
	char mapped;
	
	switch (c) /* remapping */
	{
		case 0x23:	/* livre */
			mapped = 0xa3;
			break;

		case 0x40:	/* a accent grave */
			mapped = 0x88;
			break;

		case 0x5b:	/* degre */
			mapped = 0xa1;
			break;

		case 0x5c:	/* c cedille */
			mapped = 0x8d;
			break;

		case 0x5d:	/* paragraphe */
			mapped = 0xa4;
			break;

		case 0x5e:	/* arrow up (in fact 'dagger') */
			mapped = 0xa0;
			break;

		case 0x7b:	/* e accent aigu */
			mapped = 0x8e;
			break;

		case 0x7c:	/* u accent grave */
			mapped = 0x9d;
			break;

		case 0x7d:
			mapped = 0x8f;
			break;/* e accent grave */

		case 0x7e:	/* guillemets */
			mapped = '"';
			break;

		default:
			mapped = c;
			break;
	}
	fAffiChageEcran->AfficheBrut((unsigned char)mapped);
/* variables fColonne & fLigne */
	fColonne++;
	if (fColonne > 80)
		fColonne = 80;
	else
		fAffiChageEcran->PositionLC(fLigne, fColonne);
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::Lig00()
{
	if (fRouleauFlag)
	{
		ScrollBas();
		fLigne = 1;
	}
	else
		fLigne = 24;
	fAffiChageEcran->PositionLC(fLigne, fColonne);
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::Lig25()
{
	if (fRouleauFlag)
	{
		ScrollHaut();
		fLigne = 24;
	}
	else
		fLigne = 1;
	fAffiChageEcran->PositionLC(fLigne, fColonne);
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::ScrollBas()
{
	fAffiChageEcran->ScrollPart(1, 23, 1, -1);    
}

//------------------------------------------------------------------------

void Iso6429ScreenModule::ScrollHaut()
{
	fAffiChageEcran->ScrollPart(2, 24, 1, 1);    
}
