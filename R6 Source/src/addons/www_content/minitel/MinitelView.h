/*************************************************************************
/
/	MinitelView.h
/
/	Written by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#ifndef _MINITEL_VIEW_H_
#define _MINITEL_VIEW_H_

#include <View.h>


/* comment out this #define to disable the keyboard panel - RMP */
#define USE_KEYBOARD_PANEL

/* comment out this #define to disable the GUI preference panel - RMP */
//#define USE_PREFERENCE_PANEL


#define kDISCONNECT			'KiLL'


class	Protocole;


#define kG0_NORM			  1
#define kG0_DBHAUT			  2
#define kG0_DBLARG			  3
#define kG0_DBTAIL			  4
#define kG1_NORM			  5
#define kDSJT_OFF			 10

#define kCURSOR_ON			  1
#define kCURSOR_OFF			  0
#define kCURSOR_PAUSE		 -1

#define kMIN_COL			  1
#define kMIN_LIG			  0
#define kMAX_LIG			 24
#define kMAX_COL			 80


enum	COULORS
{
	eNoir = 0,
	eRouge,
	eVert,
	eJaune,
	eBleu,
	eMagenta,
	eCyan,
	eBlanc
};

struct	mem_car
{
	int32			col_fnd;
	int32			col_txt;
	int32			pol;
	bool			inv;
	bool			masq;
	bool			soul;
	int16			delim;
	unsigned char	car;
};


//========================================================================

class MinitelView : public BView
{
	public:
							MinitelView			(BRect);
							~MinitelView		();
		virtual	void		AllAttached			();
    	virtual void		AttachedToWindow	();
   		virtual void		Draw				(BRect);
   		virtual void		KeyDown				(const char* bytes,
   												 int32 num_bytes);
   		virtual void		MessageReceived		(BMessage*);
   		virtual void		MouseDown			(BPoint);
   		virtual void		Pulse				();

		void				Affiche				(unsigned char c);			// affichage teletel (avec attributs de zone)
		void				AfficheBrut			(unsigned char c);			// affichage mode mixte (attributs caracteres uniquement)
		void				AfficheCount		(unsigned char c,			// affichage avec repetition mode teletel
												 int32 nbre);
		void				DelCar				(int32 coldeb,				// supp caracteres
												 int32 nbre);
		void				Efface				();
		void				EffaceDebEcran		();
		void				EffaceDebLigne		();
		void				EffaceFinEcran		();
		void				EffaceFinLigne		();
		void				EffaceLignes		(int32 lignedeb,			// effacmt lignes
												 int32 lignefin);
		void				EnableCursor		(bool);
		void				Init40Col			();
		void				Init80Col			();
		void				InsCar				(int32 coldeb,				// insert caracteres (espaces)
												 int32 nbre);
		void				PoliceCaracteres	(int32 pol)
													{ fCurPol = pol; };
		void				PositionLC			(int32 ligne,				// affectation position courante
												 int32 colonne);
		void				ScrollPart			(int32 lignedeb,			// scroll dans les deux sens
												 int32 lignefin,
												 int32 nbre,
												 int32 sens);
		void				SetAttZone			(int32 color,				// chgt couleur fond, masq, soul / zone
												 int32 masq_flag,
												 int32 soul_flag);
		void				SetFndColor			(int32 color)
													{ fCurFnd = color; };
		void				SetInverse			(bool flag)
													{ fInverseFlag = flag; };
		void				SetPageRevele		(bool flag)
													{ fPageRevele = flag; };
		void				SetRangee0			(int32 status);				// connecte/fin
		void				SetSouligne			(bool flag)
													{ fSoulFlag = flag; };
		void				SetStdColor			();
		void				SetStdGrey			();
		void				SetTxtColor			(int32 color)
													{ fCurTxt = color; };
		void				Zoom				(float factor);

	private:
		void				AffiCursor			();
		void				Couleur				(int32 color);
		void				DoAff				(struct mem_car c);
		void				DoAff				(struct mem_car c,
												 int32 lig,
												 int32 col);
		void				DoAttZone			(int32 color,				// aff attributs / zone
												 int32 masq_flag,
												 int32 soul_flag);
		void				DrawArea			(int32 l,
												 int32 c_min,
												 int32 c_max);
		int32				DrawChar			(mem_car cc,
												 int32 l,
												 int32 c);
		void				DrawRangee0			(int32);
		void				FillCharMask		(mem_car cc);
		void				HideCursor			(bool flag);
		void				NormPos				();
		void				Reset				();
		void				Resize				();
		void				RestoreAtt			();							// restore att zone si delimiteur ecrase
		void				SetCellSize			(int32 x,
												 int32 y);

		bool				fDraw;
		bool				fInverseFlag;
		bool				fSoulFlag;
		bool				fPageRevele;
		float				fCsteTaille;
		float				fRapLarg;
		int16				fSelBox;
		int32				fCurCol;
		int32				fCurFnd;
		int32				fCurLig;
		int32				fCurPol;
		int32				fCurTxt;
		int32				fCursorFlag;
		int32				fCursorState;
		int32				fCX;
		int32				fCY;
		int32				fMaxCol;
		int32				fOldStatus;
		BBitmap*			fCharMask;
		BBitmap*			fCharMaskDH;
		BBitmap*			fCharMaskDS;
		BBitmap*			fCharMaskDW;
		BBitmap*			fCharTable;
		rgb_color			fCouleurs[8];
		struct mem_car		fMemPages[kMAX_LIG + 1][kMAX_COL + 1];
		struct mem_car		fSaveCursor;
		Protocole*			fProtocole;
#ifdef USE_PREFERENCE_PANEL
		BWindow*			fPreferenceWindow;
#endif
};

		status_t			ConnectThread(void*);


//========================================================================

#ifdef USE_KEYBOARD_PANEL

#include <Box.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <TextControl.h>


#define kKEYBOARD_BAR_HEIGHT	 57


class KeyboardPanel : public BBox
{
	public:
							KeyboardPanel		(BRect, MinitelView*);
							~KeyboardPanel		();
		void				AttachedToWindow	();
   		void				MessageReceived		(BMessage*);

	private:
		void				BuildMenu			();
		void				CheckMenus			();
		int32				FindMenuItem		(const char*);
		void				SaveMenu			();

		bool				fChanged;
		BFile*				fMinitelDirectory;
		BMenuField*			fMenuField;
		BMenuItem*			fDelete;
		BMenuItem*			fSave;
		BPopUpMenu*			fMenu;
		BTextControl*		fPhoneText;
		MinitelView*		fView;
};

#endif	/* USE_KEYBOARD_PANEL */


//========================================================================

#ifdef USE_PREFERENCE_PANEL

#include "Window.h"

class PreferenceWindow : public BWindow
{
	public:
							PreferenceWindow	(MinitelView*);
		void				MessageReceived		(BMessage* msg);

	private:
		MinitelView*		fMinitel;
};

#endif	/* USE_PREFERENCE_PANEL */

#endif	/* _MINITEL_VIEW_H_ */
