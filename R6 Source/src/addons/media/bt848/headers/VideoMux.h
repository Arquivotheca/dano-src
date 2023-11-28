/*
	
	VideoMux.h
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _VIDEO_MUX_H
#define _VIDEO_MUX_H

#include <SupportDefs.h>

#include "VideoDefs.h"

class BVideoMux
{
public:
							BVideoMux(const char *name);
virtual						~BVideoMux();
							
virtual char *				Name();
virtual uint32				NumberInputs();
virtual	status_t			SetSource(uint32 source);
virtual	uint32				Source() const;

private:

virtual	void				_ReservedVideoMux1();
virtual	void				_ReservedVideoMux2();
virtual	void				_ReservedVideoMux3();

							BVideoMux(const BVideoMux &);
		BVideoMux			&operator=(const BVideoMux &);

		uint32				fSource;
		char 				fName[32];
		uint32				_reserved[3];

};

#endif


