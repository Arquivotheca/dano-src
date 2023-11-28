//========================================================================
//	MPathPopup.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPATHPOPUP_H
#define _MPATHPOPUP_H

#include <PopUpMenu.h>


class MPathPopup : public BPopUpMenu
{
public:
								MPathPopup(
									const char *	inTitle,
									entry_ref&		inRef);
								~MPathPopup();

		void					OpenItem(
									BMenuItem*	inItem);

};


class MPathMenu : public BMenu
{
public:
								MPathMenu(
									const char *	inTitle,
									entry_ref&		inRef);
								~MPathMenu();

		void					OpenItem(
									BMenuItem*	inItem);

};

#endif
