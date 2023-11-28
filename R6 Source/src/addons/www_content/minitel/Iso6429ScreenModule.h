/*************************************************************************
/
/	Iso6429ScreenModule.h
/
/	Modified by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#ifndef _ISO6429_SCREEN_MODULE_H_
#define _ISO6429_SCREEN_MODULE_H_

#include "VirtScreenModule.h"


class	MinitelView;
class	Protocole;


/* etats speciaux attente argument */
#define kESCAPE			 1
#define kPOSIT_1		 3
#define kPOSIT_2		 4
#define kESC_CSI		12
#define kIGNORE			99
#define kMAX_CSI_COUNT	25


//========================================================================

class Iso6429ScreenModule : public VirtScreenModule
{
	public:
							Iso6429ScreenModule	(Protocole*,
												 MinitelView*);
		int32				GetColonne			()
													{ return fColonne; };
		int32				GetLigne			()
													{ return fLigne; };
		void				ModeRlx				(int32 flag)
													{ fRouleauFlag = flag; };
		void				OutScr				(unsigned char s);	/* interface ecran */
		void				Reset				();
		void				SetConnectStat		(int32 status);

		void				AffEcran			(unsigned char c);  /* affiche caractere  */
		void				DoCan				();					/* affichage pave plein */
		void				DoCsi				(unsigned char s);
		void				DoEsc				(unsigned char s);
		void				DoPos				();
		void				DoSpe				(unsigned char s);
		void				JeuG0				(unsigned char c);	/* jeu americain */
		void				JeuG1				(unsigned char c);  /* jeu francais */
		void				Lig00				();
		void				Lig25				();
		void				ScrollBas			();
		void				ScrollHaut			();

		MinitelView*		fAffiChageEcran;
		Protocole*			fProtocole;

	protected:
		int32				fColonne;
		int32				fCsiArg[kMAX_CSI_COUNT];
		int32				fCsiCount;
		int32				fFndCol;
		int32				fInvFlag;
		int32				fJeu;
		int32				fLigne;
		int32				fRouleauFlag;
		int32				fSauveColonne;
		int32				fSauveFndCol;
		int32				fSauveInvFlag;
		int32				fSauveJeu;
		int32				fSauveLigne;
		int32				fSauveSoulFlag;
		int32				fSauveTxtCol;
		int32				fSoulFlag;
		int32				fSpe;
		int32				fTxtCol;
		int32				fX;
		int32				fY;
};
#endif
