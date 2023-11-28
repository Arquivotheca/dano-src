/*
	
	Bt848VideoMux.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
	
*/

#ifndef BT848_VIDEO_MUX_H
#define BT848_VIDEO_MUX_H

#include <unistd.h>
#include <bt848_driver.h>

#include "VideoMux.h"
#include "Bt848Source.h"

class Bt848VideoMux : public BVideoMux
{
public:

							Bt848VideoMux(	const char *name,
											uint32 bt848,
											bt848_config *config);
							~Bt848VideoMux();
							
		uint32				NumberInputs();
		status_t			SetSource(uint32 source);

private:

							Bt848VideoMux(const Bt848VideoMux &);
		Bt848VideoMux		&operator=(const Bt848VideoMux &);

		void				SetColorBars(bool setting);
		bool				ColorBars() const;

		uint32 				fBt848;
		bt848_config 		*fConfig;
};

#endif


