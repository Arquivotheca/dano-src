//========================================================================
//	MBoxControlChild.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MBOXCONTROLCHILD_H
#define _MBOXCONTROLCHILD_H

#include <View.h>
#include "MTargeter.h"


class MBoxControlChild : public BView, public MTargeter
{
public:
								MBoxControlChild(
									BRect 		inArea,
									const char*	inName);
								~MBoxControlChild();

		void					DrawFrame();
virtual	void					MouseDown(
									BPoint where);
virtual	void					MakeFocus(
									bool focusState = TRUE);
		void					Invoke();

		void					SetEnabled(
									bool on);
		bool					IsEnabled() const;
		
private:

		bool					fEnabled;
};

inline bool MBoxControlChild::IsEnabled() const
{
	return fEnabled;
}

#endif
