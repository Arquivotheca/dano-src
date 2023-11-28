/*************************************************************************
/
/	StdScreenModule.h
/
/	Modified by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#ifndef _STD_SCREEN_MODULE_H_
#define _STD_SCREEN_MODULE_H_

#include "VirtScreenModule.h"


class	MinitelView;


/* etats speciaux attente argument */
#define kESCAPE			  1
#define kREPEAT			  2
#define kPOSIT_1		  3
#define kPOSIT_2		  4
#define kSS2_1			  5
#define kSS2_2			  6
#define kSOH			  7
#define kTRANSPARENT_1	  8
#define kTRANSPARENT_2	  9
#define kTRANSPARENT_3	 10
#define kTRANSPARENT_4	 11
#define kESC_CSI		 12
#define kMASQUAGE_1		 13
#define kMASQUAGE_2		 14
#define kISO_2022_1		 15
#define kISO_2022_2		 16
#define kISO_2022_3		 17
#define kIGNORE			 99
#define kMAX_CSI_COUNT	 25


//========================================================================

class StdScreenModule : public VirtScreenModule
{
	public:
							StdScreenModule		(MinitelView*);
		void				Reset				();
		void				ResAttCar			();
		void				ResAttZone			();
		void				HomeEcran			();					/* separateur d'articles ou sous-articles*/
		void				InitEcran			();					/* SET_UP ecran */
		void				NormeColonne		();
		void				NormeColonneH		();
		void				Col41				(int32 h);
		void				Col00				();
		void				ModeRlx				(int32 flag);
		void				ScrollHaut			();
		void				ScrollBas			();
		void				NormeLigne			();
		void				Lig25				();
		void				Lig00				();
		void				JeuG0				(unsigned char c);	/* jeu standard */
		void				JeuG1				(unsigned char c);	/* alphamosaique */
		void				AffEcran			(unsigned char c);	/* affiche caractere  */
		void				OutScr				(unsigned char s);	/* interface ecran */
		void				DoSpace				();					/* traitement espace = "blanc" + delimiteur de zone */
		void				DoSpe				(unsigned char s);
		void				DoCsi				(unsigned char s);
		void				DoExt				(unsigned char s);	/* extended */
		void				DoAcc				(unsigned char s);	/* accents */
		void				DoEsc				(unsigned char s);
		void				DoPos				();
		void				DoRep				(int32 nbre);		// repetition dernier caractere
		void				RealDoRep			(int32 nbre);		// real_stuff of repetition dernier caractere
		void				DoCan				();					// effacement fin ligne code 0x18
		int32				GetLigne			();
		int32				GetColonne			();
		void				SetConnectStat		(int32 status);
		void				Ligne0ModeMixte		(int32 col);

protected:
		int32				fColonne;
		int32				fCsiArg[kMAX_CSI_COUNT];
		int32				fCsiCount;
		int32				fDelim;
		int32				fFndCol;
		int32				fGraph;
		int32				fHauteur;
		int32				fInvFlag;
		int32				fLargeur;
		int32				fLigne;
		int32				fMasqFlag;
		int32				fMaxColLig0;
		int32				fRouleauFlag;
		int32				fSauveColonne;
		int32				fSauveDelim;
		int32				fSauveFndCol;
		int32				fSauveGraph;
		int32				fSauveHauteur;
		int32				fSauveInvFlag;
		int32				fSauveLargeur;
		int32				fSauveLigne;
		int32				fSauveMasqFlag;
		int32				fSauveSoulFlag;
		int32				fSauveTxtCol;
		int32				fSoulFlag;
		int32				fSpe;
		int32				fTxtCol;
		int32				fX;
		int32				fY;
		unsigned char		fLast;
		unsigned char		fSauveLast;
		unsigned char		fAcc;
		MinitelView*		fAffichageEcran;
};
#endif
