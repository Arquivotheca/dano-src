/*	$Id: DPulseView.h,v 1.1 1998/10/21 12:03:02 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/23/98 16:46:49
*/

#ifndef DPULSEVIEW_H
#define DPULSEVIEW_H

class DPulseView : public BView {
public:
		DPulseView() : BView(BRect(0, 0, 0, 0), "pulsar", 0, B_PULSE_NEEDED) {};
virtual	void Pulse();
};

#endif
