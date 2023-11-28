/*	$Id: DPulseView.cpp,v 1.2 1998/11/17 12:16:37 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/23/98 16:47:27
*/

#include "bdb.h"
#include "DPulseView.h"
#include "DMessages.h"

#include <Window.h>

void DPulseView::Pulse()
{
	Window()->PostMessage(kMsgPulse);
} /* DPulseView::Pulse */
