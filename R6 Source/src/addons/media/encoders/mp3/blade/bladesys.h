/*
			(c) Copyright 1998, 1999 - Tord Jansson
			=======================================

		This file is part of the BladeEnc MP3 Encoder, based on
		ISO's reference code for MPEG Layer 3 compression.

		This file doesn't contain any of the ISO reference code and
		is copyright Tord Jansson (tord.jansson@swipnet.se).

	BladeEnc is free software; you can redistribute this file
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

*/

#ifndef	BLADESYS_DOT_H
#define	BLADESYS_DOT_H

extern	int		setPriority( char * pPrioString );

extern	void	prepStdin( void );
extern	void	prepStdout( void );

extern	int		be_kbhit();
extern	int		be_getch();
#ifdef	WILDCARDS
extern	void *expandWildcards( int * pArgc, char ** pArgv[] );
extern	void	freeExpandWildcardMem( void * pArgLink );
#endif

#endif

