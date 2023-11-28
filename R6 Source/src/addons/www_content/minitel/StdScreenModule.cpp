/*************************************************************************
/
/	StdScreenModule.cpp
/
/	Modified by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#include "StdScreenModule.h"
#include "MinitelView.h"


//========================================================================

StdScreenModule::StdScreenModule(MinitelView* view)
	: VirtScreenModule	(),
	fColonne			(1),
	fDelim				(0),
	fGraph				(0),
	fHauteur			(1),
	fInvFlag			(0),
	fLargeur			(1),
	fLigne				(0),
	fMaxColLig0			(40),
	fRouleauFlag		(0),
	fSauveColonne		(1),
	fSauveDelim			(0),
	fSauveFndCol		(eNoir),
	fSauveGraph			(0),
	fSauveHauteur		(1),
	fSauveInvFlag		(0),
	fSauveLargeur		(1),
	fSauveLigne			(0),
	fSauveMasqFlag		(0),
	fSauveSoulFlag		(0),
	fSauveTxtCol		(eBlanc),
	fSpe				(0),
	fTxtCol				(eBlanc),
	fSauveLast			(' '),
	fAffichageEcran		(view)
{
}

//------------------------------------------------------------------------

void StdScreenModule::Reset()
{
	fMaxColLig0 = 40;
	fAffichageEcran->Init40Col();
	InitEcran();
	fLigne = 0;
	fColonne = 1;
	fSauveLigne = 0;
	fSauveColonne = 1;
	fSauveDelim = 0;
	fSauveTxtCol = eBlanc;
	fSauveFndCol = eNoir;
	fSauveMasqFlag = 0;
	fSauveSoulFlag = 0;
	fSauveInvFlag = 0;
	fSauveHauteur = 1;
	fSauveLargeur = 1;
	fSauveGraph = 0;
	fSauveLast = ' ';	
	fDelim = 0;
	fTxtCol = eBlanc;
	fInvFlag = 0;
	fLargeur = 1;
	fHauteur = 1;
	fGraph = 0;
	fRouleauFlag = 0;
	fSpe = 0;
}

//------------------------------------------------------------------------

void StdScreenModule::ResAttCar()
/* reset attributs paralleles (niveau caractere) */
{				/* separateur d'articles ou sous-articles */
	fDelim = 0;
	fTxtCol = eBlanc;
	fAffichageEcran->SetTxtColor(fTxtCol);
	fInvFlag = 0;
	fAffichageEcran->SetInverse(false);
	fLargeur = 1;
	fHauteur = 1;
	fGraph = 0;
	fAffichageEcran->PoliceCaracteres(kG0_NORM);
}

//------------------------------------------------------------------------

void StdScreenModule::ResAttZone()
/* reset attributs serie (niveau zone) */
{
	fFndCol = eNoir;
	fMasqFlag = 0;
	fSoulFlag = 0;
}

//------------------------------------------------------------------------

void StdScreenModule::HomeEcran()
/* retour debut ecran = separateur articles */
{
	ResAttCar();
	ResAttZone();
//	fAffichageEcran->EnableCursor(false);
	fAffichageEcran->SetPageRevele(false);
	fLigne = 1;
	fColonne = 1;
	fAffichageEcran->PositionLC(fLigne, fColonne);    
}

//------------------------------------------------------------------------

void StdScreenModule::InitEcran()
/* SET_UP ecran */
{
	fAffichageEcran->Efface();    
	HomeEcran();
}

//------------------------------------------------------------------------

void StdScreenModule::NormeColonne()
/* norme colonne version 1 */
{
	if (fColonne > 40)
		Col41(1);	/* si debordement > ajout 1 ligne */
	if (fColonne < 1)
		Col00();
	fAffichageEcran->PositionLC(fLigne, fColonne);    
}

//------------------------------------------------------------------------

void StdScreenModule::NormeColonneH()
/* norme colonne version 2 */
{
	if (fColonne > 40)
		Col41(fHauteur);	/* si debordement > ajout hauteur ligne(s) */
	if (fColonne < 1)
		Col00();
	fAffichageEcran->PositionLC(fLigne, fColonne);    
}

//------------------------------------------------------------------------

void StdScreenModule::Col41(int32 h)
{
	if (fLigne == 0) 
	{
		if (fColonne > fMaxColLig0)
			fColonne = fMaxColLig0;
	}
	else
	{
		fColonne = 1;
		fLigne += h;
		NormeLigne();
	}
}

//------------------------------------------------------------------------

void StdScreenModule::Col00()
{
	if (fLigne == 0)
		fColonne = 1;
	else
	{
		fColonne = 40;
		fLigne--;
		NormeLigne();
	}
}

//------------------------------------------------------------------------

void StdScreenModule::ModeRlx(int32 flag)
{
	fRouleauFlag = flag;
}

//------------------------------------------------------------------------

void StdScreenModule::ScrollHaut()
{
	fAffichageEcran->ScrollPart(2, 24, fHauteur, 1);    
}

//------------------------------------------------------------------------

void StdScreenModule::ScrollBas()
{
	fAffichageEcran->ScrollPart(1, 23, fHauteur, -1);    
}

//------------------------------------------------------------------------

void StdScreenModule::NormeLigne()
{
	if (fLigne > 24)
		Lig25();
	if (fLigne < 1)
		Lig00();
	fAffichageEcran->PositionLC(fLigne, fColonne);    
}

//------------------------------------------------------------------------

void StdScreenModule::Lig25()
{
	if (fRouleauFlag)
	{
		ScrollHaut();
		fLigne = 24;
	}
	else
		fLigne -= 24;
}

//------------------------------------------------------------------------

void StdScreenModule::Lig00()
{
	if (fRouleauFlag)
	{
		ScrollBas();
		fLigne = 1;
	}
	else
		fLigne = 24;
}

//------------------------------------------------------------------------

void StdScreenModule::JeuG0(unsigned char c)
/* jeu standard */
{
	char mapped;
	
	switch (c) /* remapping */
	{
		case 0x5e:	/* arrow up */
			mapped = '^';
			break;

		default:
			mapped = c;
			break;
	}
	
/* sortie ecran 
 * pb si DBLE LARG et fColonne = 40 !!!!
 * pb si DBLE HAUT et ligne = 0 ou 1 !!!!
 */
	fAffichageEcran->Affiche((unsigned char)mapped);
/* variables fColonne & ligne */
	fColonne += fLargeur;
	NormeColonneH();
}

//------------------------------------------------------------------------

void StdScreenModule::JeuG1(unsigned char c)
/* alphamosaique */
{
/* graphique = delimiteur couleur de fond quoi qu'il arrive */
	if (fColonne == 1)
		fAffichageEcran->SetAttZone(fFndCol, 0, 0);
	else
		fAffichageEcran->SetAttZone(fFndCol, -1, -1);

	if ((c > 0x3f) && (c < 0x5f))
	{
//		DPRINTF_E("Caractere graphique inconnu 0x%x > 0x%x\n",(unsigned int)c,(unsigned int)c+0x20);
		c = c + 0x20;
	}

	fAffichageEcran->Affiche((unsigned char)c);
	/* variables fColonne & ligne */
	fColonne++;
	NormeColonne();
}

//------------------------------------------------------------------------

void StdScreenModule::DoSpace()
/* traitement espace = "eBlanc" + delimiteur de zone */
{
	if (fDelim) 
	{
		fAffichageEcran->SetAttZone(fFndCol, fMasqFlag, fSoulFlag);
		fDelim = 0;
	}
	else if (fGraph)
		fAffichageEcran->SetAttZone(fFndCol, -1, -1);

	JeuG0((unsigned char)0x20);
}

//------------------------------------------------------------------------

void StdScreenModule::AffEcran(unsigned char c)
/* affiche caractere  */
{
	if (c == 0x20) 
	{
		DoSpace();
	}
	else	
	{
		if (fGraph)
			JeuG1(c);
		else
			JeuG0(c);
	}
}

//------------------------------------------------------------------------

void StdScreenModule::OutScr(unsigned char s)
/* interface ecran */
{
//fprintf(stderr, "StdScreenModule::OutScr() begin\n");
	if (fSpe != 0)
		DoSpe(s);
	else
	{
		if ((s >= 0x20) && (s <= 0x7f))
		{
			AffEcran(s);
			fLast = s;
		}
		else /* controles */
		{
			switch (s)
			{
				case 0x0:	/* NUL = bourrage */
				case 0x1:	/* ??? */
				case 0x4:	/* ??? */
					break;

				case 0x7:	/*NXBeep();*/
					break;	/* BEL = cloche */

				case 0x8: 	/* BS = gche */
					fColonne--;
					NormeColonne();
					break;

				case 0x9:	/* HT = dte */
					fColonne++;
					NormeColonne();
					break;

				case 0xA:
					if (fLigne == 0)	/* LF ligne 0 : retour ecran */
					{
						fFndCol = fSauveFndCol;
						fMasqFlag = fSauveMasqFlag;
						fSoulFlag = fSauveSoulFlag;
						fDelim = fSauveDelim;
						fLast = fSauveLast;

						fTxtCol = fSauveTxtCol;
						fAffichageEcran->SetTxtColor(fTxtCol);

						fInvFlag = fSauveInvFlag;
						if (fInvFlag)
							fAffichageEcran->SetInverse(true);
						else
							fAffichageEcran->SetInverse(false);

						fLargeur = fSauveLargeur;
						fHauteur = fSauveHauteur;
						fGraph = fSauveGraph;
						if (fGraph)
							fAffichageEcran->PoliceCaracteres(kG1_NORM);
						else
							fAffichageEcran->PoliceCaracteres(fHauteur + (fLargeur - 1) * 2);

						fLigne = fSauveLigne;
						fColonne = fSauveColonne;
						fAffichageEcran->PositionLC(fLigne, fColonne);
					}
					else			/* LF = bas */
					{
						fLigne++;
						NormeLigne();
					}
					break;

				case 0xB:	/* VT = haut */
					if (fLigne != 0 )
					{
						fLigne--;
						NormeLigne();
					}
					break;

				case 0xC:	/* FF = init */
					InitEcran();
					break;

				case 0xD:	/* CR */
					fColonne = 1;
					fAffichageEcran->PositionLC(fLigne, fColonne);
					break;

				case 0xE:	/* SO = graph on */
					fGraph = 1;
					fLargeur = 1;
					fHauteur = 1;
					fAffichageEcran->SetInverse(false);
					fAffichageEcran->SetSouligne(false);
					fSoulFlag = 0;
					fAffichageEcran->PoliceCaracteres(kG1_NORM);
					break;

				case 0xF:	/* SI = graph off */
					fGraph = 0;
					fAffichageEcran->SetSouligne(false);
					fSoulFlag = 0;
					fAffichageEcran->PoliceCaracteres(kG0_NORM);
					break;

				case 0x11:	/* Con = cursor on */
					fAffichageEcran->PositionLC(fLigne, fColonne);
					fAffichageEcran->EnableCursor(true);
					break;

				case 0x12:	/* Rep = repeat n last char */
					fSpe = kREPEAT;
					break;

				case 0x14:	/* Coff = cursor off */
					fAffichageEcran->EnableCursor(false);
					break;

				case 0x18:	/* CAN = effcmt fin ligne */
					DoCan();
					break;

				case 0x16:	/* pourquoi ???? */
				case 0x19:	/* SS2 = jeu etendu */
					if (fGraph)
						break;
					else
						fSpe = kSS2_1; 
					break;

				case 0x1B:	/* ESC */
					fSpe = kESCAPE;
					break;

				case 0x1E:	/* RS = home */
					HomeEcran();
					break;

				case 0x1F:	/* US = position y x */
					fSpe = kPOSIT_1;
					break;

				case 0x10:	/* DLE = transparence */
				case 0x13:	/* Sep = separateur */
				case 0x1D:	/* SS3 = jeu non supporte */
					//DPRINTF_E("SPE Ox%x non supporte\n",(unsigned int)s);
					fSpe = kIGNORE;
					break;			/* ??? : ignore suivant */

				default:
					//DPRINTF_E("CTL Ox%x non supporte\n",(unsigned int)s);
					break;
			} /* end of switch */
		}
	}
//fprintf(stderr, "StdScreenModule::OutScr() end\n");
}

//------------------------------------------------------------------------

void StdScreenModule::DoSpe(unsigned char s)
/* caracteres speciaux a argument(s) */
{
	switch (fSpe)
	{
		case kESCAPE:
			fSpe = 0;
			DoEsc(s);
			break;
					
		case kREPEAT:
			DoRep(s - 0x40);
			fSpe = 0;
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
				return ;
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
				return ;
			}
			break;

		case kSS2_1:
			if ((s > 0x40) && (s < 0x4f))
			{
				fSpe = kSS2_2;
				fAcc = s;
				break; /* accent */
			}
			else
			{
				DoExt(s);
				fSpe=0;
				break;
			}

		case kSS2_2:
			DoAcc(s);
			fSpe = 0;
			break;

		case kESC_CSI:
			if ((s >= 0x30) && (s <= 0x3f))
			{
				if (fCsiCount < kMAX_CSI_COUNT) 
				{
					fCsiArg[fCsiCount]=s;
					fCsiCount++;
				}
				else
				{
					// DPRINTF_E("CSI ERROR COUNT\n");
					DoCsi(0);
				}
			}
			else
				DoCsi(s);
			break;

		case kSOH:
			if (s == 0x04)
				fSpe = 0;
			break; /* transmission donnees */

		case kTRANSPARENT_1:
			if (s == 0x1b)
				fSpe = kTRANSPARENT_2;
/*
			else
				DPRINTF_E("TRANSPARENT\n");
*/
			break; /* transparent ecran : attente ESC */								  

		case kTRANSPARENT_2:
			switch (s)
			{
				case 0x25:
					fSpe = kTRANSPARENT_3;
					break;

				case 0x2f:
					fSpe = kTRANSPARENT_4;
					break;

				default:
					fSpe = kTRANSPARENT_1;
					break;
			}
			break; /* transparent ecran : apres ESC attente 2/5 ou 2/f */		

		case kTRANSPARENT_3:
			if (s == 0x40)
				fSpe = 0;
			else
				fSpe = kTRANSPARENT_1;
			break; /* transparent ecran : ESC 2/5 recu attente 4/0 */     

		case kTRANSPARENT_4:
			if (s == 0x3f)
				fSpe = 0;
			else
				fSpe = kTRANSPARENT_1;
			break; /* transparent ecran : ESC 2/f recu attente 3/f */

		case kMASQUAGE_1:
			if (s == 0x20)
				fSpe = kMASQUAGE_2;
			else
				fSpe = 0;
			break; /* masquage ecran : ESC 2/3 2/0 */

		case kMASQUAGE_2:
			switch(s)
			{
				case 0x58:
				 	fAffichageEcran->SetPageRevele(false);
					break;

				case 0x5f:
				 	fAffichageEcran->SetPageRevele(true);
					break;

				default:
					break;
			}
			fSpe = 0;
			break; /* masquage ecran : ESC 2/3 2/0 5/8 ou 5/f*/
					
		case kISO_2022_1:
			if (s < 0x20)
			{		/* resynchro */
				fSpe = 0;
				OutScr(s);
				break;
			}
			else if ((s >= 0x20) && (s <= 0x2f))
			{
				fSpe = kISO_2022_2;
				break;
			}
			else
				fSpe = 0;
			break; /* norme ISO 2022 */

		case kISO_2022_2:
			if (s < 0x20)
			{		/* resynchro */
				fSpe = 0;
				OutScr(s);
				break;
			}
			else if ((s >= 0x20) && (s <= 0x2f))
			{
				fSpe = kISO_2022_3;
				break;
			}
			else
				fSpe = 0;
			break; /* norme ISO 2022 */

		case kISO_2022_3:
			if (s < 0x20)
			{		/* resynchro */
				fSpe = 0;
				OutScr(s);
				break;
			}
			else
				fSpe = 0;
			break; /* norme ISO 2022 */
					
		case kIGNORE:
			fSpe = 0;
			break; /* ignore */
	}
}

//------------------------------------------------------------------------

void StdScreenModule::DoCsi(unsigned char s)
{
	int32	i;
	int32	count;
	int32	puissance;
	
	fSpe = 0;
	if (fLigne == 0)
		return;
	
	count = 0;
	puissance = 1;
	if (fCsiCount != 0)
	{
		for (i = fCsiCount; i > 0;i--)
		{
			if ((fCsiArg[i - 1] >= 0x30) && (fCsiArg[i - 1] <= 0x3f))
			{
				count = count + (fCsiArg[i - 1] - 0x30) * puissance;
				puissance = puissance * 10;
			}
			else
				puissance = 0;
		}
	}

	switch (s)
	{
		case 'A':	/* dep vers le haut n fois */
			fLigne = fLigne - count;
			if (fLigne < 1)
				fLigne = 1;
			fAffichageEcran->PositionLC(fLigne, fColonne);    
			break;
	  
		case 'B':	/* dep vers le bas n fois */
			fLigne = fLigne + count;
			if (fLigne > 24)
				fLigne = 24;
			fAffichageEcran->PositionLC(fLigne, fColonne);    
			break;
	  
		case 'C':	/* dep vers la droite n fois */
			fColonne = fColonne + count;
			if (fColonne > 40)
				fColonne = 40;
			fAffichageEcran->PositionLC(fLigne, fColonne);    
			break;
			  
		case 'D':	/* dep vers la gauche n fois */
			fColonne = fColonne - count;
			if (fColonne < 1)
				fColonne = 1;
			fAffichageEcran->PositionLC(fLigne, fColonne);    
			break;
	  
		case 'H':	/* pos curseur */
			if ((fCsiCount < 3) || (fCsiCount > 5))
				break;
			fY = 0;
			fX = 1;
			switch (fCsiCount)
			{
				case 3:
					if (fCsiArg[1] == 0x3B)
					{
						fY = fCsiArg[0] - 0x30;
						fX = fCsiArg[2] - 0x30;
					}
					break;

				case 4:
					if (fCsiArg[1] == 0x3B)
					{
						fY = fCsiArg[0] - 0x30;
						fX = fCsiArg[2] - 0x30;
						fX = fX * 10 + fCsiArg[3] - 0x30;
					}
					else if (fCsiArg[2] == 0x3B)
					{
						fY = fCsiArg[0] - 0x30;
						fY = fY * 10 + fCsiArg[1] - 0x30;
						fX = fCsiArg[3] - 0x30;
					}
					break;

					case 5:
						if (fCsiArg[2] == 0x3B)
						{
							fY = fCsiArg[0] - 0x30;
							fY = fY * 10 + fCsiArg[1] - 0x30;
							fX = fCsiArg[3] - 0x30;
							fX = fX * 10 + fCsiArg[4] - 0x30;
						}
						break;
				}
				if (fY != 0)
				{
					fLigne = fY;
					fColonne = fX;
					NormeLigne();    
					NormeColonne();    
				}
				break;
			  
		case 'J':
			switch(fCsiArg[0] - 0x30)
			{
				case 0:
					fAffichageEcran->PositionLC(fLigne, fColonne);
					fAffichageEcran->EffaceFinEcran();
					break; /* del -> fin ecran */

				case 1:
					fAffichageEcran->PositionLC(fLigne, fColonne);
					fAffichageEcran->EffaceDebEcran();
					break; /* del -> haut ecran */

					case 2:
						InitEcran();
						break; /* cls + reste pos */

					default:
						break;
			}
			break;
				
		case 'K':
			switch(fCsiArg[0] - 0x30)
			{
				case 0:
					fAffichageEcran->PositionLC(fLigne, fColonne);
					fAffichageEcran->EffaceFinLigne();
					break; /* del -> fin ligne */

				case 1:
					fAffichageEcran->PositionLC(fLigne, fColonne);
					fAffichageEcran->EffaceDebLigne();
					break; /* del -> debut ligne */

				case 2:
					fAffichageEcran->PositionLC(fLigne, fColonne);
					fAffichageEcran->EffaceDebLigne();
					fAffichageEcran->EffaceFinLigne();
					break; /* efface ligne */

				default:
					break;
			}
			break;

		case 'L':
			fAffichageEcran->ScrollPart(fLigne, 24, count, -1);
			break; /* insertion count rangees depuis ligne courante */

		case 'M':
			fAffichageEcran->ScrollPart(fLigne + count, 24, count, 1);
			break; /* suppression count rangees depuis ligne courante */
		
		case 'P':
			fAffichageEcran->DelCar(fColonne, count);
			break;
		
		case '@':
			fAffichageEcran->InsCar(fColonne, count);
			break;

		default:
			// DPRINTF_E("CSI Ox%x non supporte\n",(unsigned int)s);
			break;
	}
}

//------------------------------------------------------------------------

void StdScreenModule::DoExt(unsigned char s)
/* extended */
{
	switch (s)
	{
		case 0x23:	/* livre */
			fLast = 0xa3;
			break;

		case 0x24:	/* dollar */
			fLast = '$';
			break;

		case 0x26:	/* diese */
			fLast = '#';
			break;

		case 0x27:	/* paragraphe */
			fLast = 0xa4;
			break;

		case 0x2c:	/* fleche gauche */
			fLast = '<';
			break;

		case 0x2d:	/* fleche haut (in fact 'dagger') */
			fLast = 0xa0;
			break;

		case 0x2e:	/* fleche droite */
			fLast = '>';
			break;

		case 0x2f:	/* fleche bas */
			fLast = '!';
			break;

		case 0x30:	/* degre */
			fLast = 0xa1;
			break;

		case 0x31:	/* +/- */
			fLast = 0xba;
			break;

		case 0x38:	/* divise */
			fLast = 0xd6;
			break;

		case 0x3c:	/* 1/4 */
			fLast = 0xd7;
			break;

		case 0x3d:	/* 1/2 */
			fLast = 0xd8;
			break;

		case 0x3e:	/* 3/4 */
			fLast = 0xd9;
			break;

		case 0x6a:	/* OE */
			fLast = 0xce;
			break;

		case 0x7a:	/* oe */
			fLast = 0xcf;
			break;

		case 0x7b:	/* beta */
			fLast = 0xa7;
			break;

		default:
			fLast = '_';
			// DPRINTF_E("SS2 Ox%x non supporte\n",(unsigned int)s);
	}

/* sortie ecran 
 * cf pb ci-dessus si DBLE LARG et fColonne = 40
 */
	fAffichageEcran->Affiche((unsigned char)fLast);
	/* variables fColonne & fLigne */
	fColonne += fLargeur;
	NormeColonneH();
}

//------------------------------------------------------------------------

void StdScreenModule::DoAcc(unsigned char s)
/* accents */
{
    switch (fAcc)
    {
        case 0x41: /* grave */
			switch (s)
			{
				case 'a':
					fLast = 0x88;
					break;

				case 'e':
					fLast = 0x8f;
					break;

				case 'u':
					fLast = 0x9d;
					break;

				default:
					fLast = s;
					break;
			}
			break;

		case 0x42: /* aigu */
			switch (s)
			{
				case 'e':
					fLast = 0x8e;
					break;

				default:
					fLast = s;
					break;
			}
			break;

        case 0x43 : /* chapeau */
			switch (s)
			{
				case 'a':
					fLast = 0x89;
					break;

				case 'e':
					fLast = 0x90;
					break;

				case 'i':
					fLast = 0x94;
					break;

				case 'o':
					fLast = 0x99;
					break;

				case 'u':
					fLast = 0x9e;
					break;

				default:
					fLast = s;
					break;
			}
			break;

        case 0x48: /* trema */
			switch (s)
			{
				case 'a':
					fLast = 0x8a;
					break;

				case 'e':
					fLast = 0x91;
					break;

				case 'i':
					fLast = 0x95;
					break;

				case 'o':
					fLast = 0x9a;
					break;

				case 'u':
					fLast = 0x9f;
					break;

				default:
					fLast = s;
					break;
			}
			break;

        case 0x4b: /* cedille */
			switch (s)
			{
			    case 'c':
			    	fLast = 0x8d;
			    	break;

			    default:
			    	fLast = s;
			    	break;
			}
			break;
	}
/* sortie ecran 
 * cf pb ci-dessus si DBLE LARG et fColonne = 40
 */
    fAffichageEcran->Affiche((unsigned char)fLast);
    /* variables fColonne & fLigne */
    fColonne += fLargeur;
    NormeColonneH();
}

//------------------------------------------------------------------------

void StdScreenModule::DoEsc(unsigned char s)
{
    int i;

    switch(s)
    {
		case 0x23:	/* mode masquage ecran */
        	fSpe = kMASQUAGE_1;
        	break;
	
		case 0x25:	/* mode transparence ecran */
        	fSpe = kTRANSPARENT_1;
        	break;
	
		case 0x20: 
		case 0x21: 
		case 0x22: 
		case 0x24: 
		case 0x26: 
		case 0x27: 
		case 0x28: 
		case 0x29: 
		case 0x2a: 
		case 0x2b: 
		case 0x2c: 
		case 0x2d: 
		case 0x2e: 
		case 0x2f: 
			fSpe = kISO_2022_1;
			break;	/* mode transparence ecran */

		case 0x35:
		case 0x36:
		case 0x37:
//			DPRINTF_E("ESC Ox%x non supporte\n",(unsigned int)s);
			fSpe = kIGNORE;
			break;	/* ignore suivant */

		case 0x40:
        	fTxtCol = eNoir;
        	fAffichageEcran->SetTxtColor(fTxtCol);
        	break;

		case 0x41:
			fTxtCol = eRouge;
			fAffichageEcran->SetTxtColor(fTxtCol);
			break;

		case 0x42:
			fTxtCol = eVert;
			fAffichageEcran->SetTxtColor(fTxtCol);
			break;

		case 0x43:
			fTxtCol = eJaune;
			fAffichageEcran->SetTxtColor(fTxtCol);
			break;

		case 0x44:
			fTxtCol = eBleu;
			fAffichageEcran->SetTxtColor(fTxtCol);
			break;

		case 0x45:
			fTxtCol = eMagenta;
			fAffichageEcran->SetTxtColor(fTxtCol);
			break;

		case 0x46:
			fTxtCol = eCyan;
			fAffichageEcran->SetTxtColor(fTxtCol);
			break;

		case 0x47:
			fTxtCol = eBlanc;
			fAffichageEcran->SetTxtColor(fTxtCol);
			break;

		case 0x48 : /* flash on */
		case 0x49 : /* flash off */
			break; /* on fait pas pour le moment... */

		case 0x4C:
			if (!fGraph)
			{
				fLargeur = 1;
				fHauteur = 1;
				fAffichageEcran->PoliceCaracteres(kG0_NORM);
			}
			break; /* NORMAL */

		case 0x4D:
			if (!fGraph)
			{
				if (fLigne > 1) /* DBLE HAUTEUR */
				{
					fLargeur = 1;
					fHauteur = 2;
					fAffichageEcran->PoliceCaracteres(kG0_DBHAUT);
				}
			}
			break;

		case 0x4E:
			if (!fGraph)
			{
				fLargeur = 2;
				fHauteur = 1;
				fAffichageEcran->PoliceCaracteres(kG0_DBLARG);
			}
			break; /* DBLE LARGEUR */

		case 0x4F:
			if (!fGraph)
			{
				if (fLigne > 1) /* DBLE LARGEUR & HAUTEUR */
				{
					fLargeur = 2;
					fHauteur = 2;
					fAffichageEcran->PoliceCaracteres(kG0_DBTAIL);
				}
				else 
				{
					fLargeur = 2;
					fHauteur = 1;
					fAffichageEcran->PoliceCaracteres(kG0_DBLARG);
				}
			}
			break;

/* attributs serie */
				/* couleur de fond */
		case 0x50:
			fFndCol = eNoir;
			fDelim = 1;
			break;

		case 0x51:
			fFndCol = eRouge;
			fDelim = 1;
			break;

		case 0x52:
			fFndCol = eVert;
			fDelim = 1;
			break;

		case 0x53:
			fFndCol = eJaune;
			fDelim = 1;
			break;

		case 0x54:
			fFndCol = eBleu;
			fDelim = 1;
			break;

		case 0x55:
			fFndCol = eMagenta;
			fDelim = 1;
			break;

		case 0x56:
			fFndCol = eCyan;
			fDelim = 1;
			break;

		case 0x57:
			fFndCol = eBlanc;
			fDelim = 1;
			break;

		case 0x58:	/* masquage on*/
			fMasqFlag = 1;
			fDelim = 1;
			break;

		case 0x5f:	/* masquage off*/
			fMasqFlag = 0;
			fDelim = 1;
			break;
		
		case 0x59:	/* souligne off */
			if (!fGraph)
			{
				fSoulFlag = 0;
				fDelim = 1; 
			}
			else
				fAffichageEcran->SetSouligne(false);
			break;

		case 0x5A: 	/* souligne on */
			if (!fGraph)
			{
				fSoulFlag = 1;
				fDelim = 1;
			}
			else
				fAffichageEcran->SetSouligne(true);
			break;
		/**/
		
		case 0x5B: /* CSI = ISO 6429 */
			fCsiCount = 0;
			for (i = 0; i < kMAX_CSI_COUNT; i++)
				fCsiArg[i] = 0;
			fSpe = kESC_CSI;
			break; /* CSI */

		case 0x5C:
			if (!fGraph)
			{
				fAffichageEcran->SetInverse(false);
				fInvFlag = 0;
			}
			break; /* inverse off */

		case 0x5D:
			if (!fGraph)
			{
				fAffichageEcran->SetInverse(true);
				fInvFlag = 1;
			}
			break; /* inverse on */

		default :
			// DPRINTF_E("ESC Ox%x non supporte\n",(unsigned int)s);
			if (s < 32)
				OutScr(s); /* essai resynchro */
			break;
    }
}

//------------------------------------------------------------------------

void StdScreenModule::DoPos()
{
	if ((fX >= 0x40) && (fY >= 0x40) && (fX <= 0x7f) && (fY <= 0x7f))
	{
		fX = fX - 0x40;
		fY = fY - 0x40;
		if ((fY == 0) && (fLigne != 0))
		{
			fSauveLigne = fLigne;
			fSauveColonne = fColonne;
			fSauveDelim = fDelim;
			fSauveTxtCol = fTxtCol;
			fSauveFndCol = fFndCol;
			fSauveMasqFlag = fMasqFlag;
			fSauveSoulFlag = fSoulFlag;
			fSauveInvFlag = fInvFlag;
			fSauveHauteur = fHauteur;
			fSauveLargeur = fLargeur;
			fSauveGraph = fGraph;
			fSauveLast = fLast;
			fDelim = 0;
			fTxtCol = eBlanc;
			fFndCol = eNoir;
		}
		fLigne = fY;
		fColonne = fX;
//		fAffichageEcran->EnableCursor(false);
		fAffichageEcran->PositionLC(fY, fX);    
		ResAttCar();
		ResAttZone();
	}
	else if ((fX >= 0x30) && (fY >= 0x30) && (fY <= 0x32) && (fX <= 0x39))
	{
		fX = fX - 0x30;
		fY = fY - 0x30;
		fY = 10 * fY + fX;
		if (fY > 24)
			return;
		fLigne = fY;
		fColonne = 1;
//		fAffichageEcran->EnableCursor(false);
		fAffichageEcran->PositionLC(fY, 1);    
		ResAttCar();
		ResAttZone();
	}
}

//------------------------------------------------------------------------

void StdScreenModule::DoRep(int32 nbre)
// repetition dernier caractere
{
	fAffichageEcran->PositionLC(fLigne, fColonne);
	RealDoRep(nbre);
	NormeColonneH();
}

//------------------------------------------------------------------------

void StdScreenModule::RealDoRep(int32 nbre)
// repetition dernier caractere
{
	char	mapped;
	int32	nbre2;
	int32	naff;
	
	if (fGraph)
	{	
		if ((fDelim) && (fLast == 0x20))
		{
			// DPRINTF_E("Delim explicite\n");
			fAffichageEcran->SetAttZone(fFndCol, fMasqFlag, fSoulFlag);
			fDelim = 0;
		}
		if ((fLast > 0x3f) && (fLast < 0x5f))
		{
			// DPRINTF_E("Caractere graphique inconnu 0x%x > 0x%x\n", (unsigned int32)fLast, (unsigned int32)fLast + 0x20);
			fLast = fLast + 0x20;
		}
		nbre2 = nbre;
		naff = 0;
		NormeColonne(); /* on sait jamais */
		while (fColonne + nbre2 > 41)
		{
			nbre2 = 41 - fColonne;
			if (nbre2 <= 0)
				break; /* on sait jamais */
//			DPRINTF_E(">>do rep1 %d fois col=%d lig=%d\n", nbre2,fColonne,fLigne);
			fAffichageEcran->SetAttZone(fFndCol, -1, -1);
			fAffichageEcran->AfficheCount(fLast, nbre2);
			fColonne = fColonne + nbre2;
			NormeColonne();
			naff = naff + nbre2;
			nbre2 = nbre - naff;
		}
//		DPRINTF_E(">>do rep2 %d fois col=%d lig=%d\n", nbre2,fColonne,fLigne);
		fAffichageEcran->SetAttZone(fFndCol, -1, -1);
		fAffichageEcran->AfficheCount(fLast, nbre2);
		/* variables fColonne & fLigne */
		fColonne = fColonne + nbre2;
	}
	else
	{
		if ((fDelim) && (fLast == 0x20))
		{
			// DPRINTF_E("Delim explicite\n");
			fAffichageEcran->SetAttZone(fFndCol, fMasqFlag, fSoulFlag);
			fDelim = 0;
		}
		switch (fLast)
		{ /* remapping */
			case 0x5e:	/* arrow up */
				mapped = '^';
				break;

			default:
				mapped = fLast;
				break;
		}
		nbre2 = nbre;
		naff = 0;
		NormeColonne(); /* on sait jamais */
		while (fColonne + fLargeur * nbre2 > 41)
		{
			nbre2 = (41 - fColonne) / fLargeur;
			if (nbre2 <= 0)
				break; /* on sait jamais */
			// DPRINTF_E("++do rep1 %d fois col=%d lig=%d\n", nbre2,fColonne,fLigne);
			fAffichageEcran->AfficheCount(fLast, nbre2);
			fColonne = fColonne + fLargeur * nbre2;
			NormeColonneH();
			naff = naff + nbre2;
			nbre2 = nbre - naff;
		}
		// DPRINTF_E("++do rep2 %d fois col=%d lig=%d\n", nbre2,fColonne,fLigne);
		if (nbre2 > 0) /* on sait jamais */
			fAffichageEcran->AfficheCount(mapped, nbre2);
		/* variables fColonne & fLigne */
		fColonne = fColonne + fLargeur * nbre2;
	}
}

//------------------------------------------------------------------------

void StdScreenModule::DoCan()
// effacement fin ligne
{
//	int32	i;
	int32	oldlig;
	int32	oldcol;
	int32	oldlarg;
	
	fAffichageEcran->PositionLC(fLigne, fColonne);
	fLast = 0x20;
	oldlig = fLigne;
	oldcol = fColonne;
	oldlarg = fLargeur;
	fLargeur = 1;
	RealDoRep(41 - fColonne);
	fLigne = oldlig;
	fColonne = oldcol;
	fLargeur = oldlarg;
	fAffichageEcran->PositionLC(fLigne, fColonne);
}

//------------------------------------------------------------------------

int32 StdScreenModule::GetLigne()
{
	return fLigne;
}

//------------------------------------------------------------------------

int32 StdScreenModule::GetColonne()
{
	return fColonne;
}

//------------------------------------------------------------------------

void StdScreenModule::SetConnectStat(int32 status)
{
	fAffichageEcran->SetRangee0(status);

	fLigne = 0;
	fColonne = 1;
	fSauveLigne = 0;
	fSauveColonne = 1;
	fSauveDelim = 0;
	fSauveTxtCol = eBlanc;
	fSauveFndCol = eNoir;
	fSauveMasqFlag = 0;
	fSauveSoulFlag = 0;
	fSauveInvFlag = 0;
	fSauveHauteur = 1;
	fSauveLargeur = 1;
	fSauveGraph = 0;
	fSauveLast = ' ';
	fAffichageEcran->EnableCursor(false);
	fAffichageEcran->PositionLC(fLigne, fColonne);
	ResAttCar();
	ResAttZone();
}

//------------------------------------------------------------------------

void StdScreenModule::Ligne0ModeMixte(int32 col)
{
	fMaxColLig0 = 80;
	fDelim = 0;
	fTxtCol = eBlanc;
	fFndCol = eNoir;
	fLigne = 0;
	fColonne = col;
	fAffichageEcran->EnableCursor(false);
	fAffichageEcran->PositionLC(fY, fX);    
	ResAttCar();
	ResAttZone();
}
