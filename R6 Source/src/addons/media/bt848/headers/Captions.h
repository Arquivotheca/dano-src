/*
	
	Captions.h
	
	Copyright 1998 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _CAPTIONS_H
#define _CAPTIONS_H
	
#include <string.h>

#include "Bt848Source.h"

/* caption modes */
#define	TEXT	0
#define	POPUP	1
#define ROLLUP2	2
#define ROLLUP3	3
#define ROLLUP4	4

/* caption decode result values */
#define NO_CAPTIONS				0
#define CAPTION_OK				1
#define CAPTION_PARITY_ERROR	2

class ClosedCaption
{
public:
				ClosedCaption(uint32 num_samples);
virtual			~ClosedCaption();
				
virtual void	Reset();
virtual	int32	Mode();
virtual	int32	Bank();
virtual	int32	DisplayBank();
virtual	int32	RollupStart();
virtual	int32	RollupEnd();
virtual	int32	CurrentLine();
virtual	char *	Line(int32 bank, int32 lineno);
virtual	char *	NetworkName();
virtual	char *	CallLetters();
virtual	char *	ProgramName();
virtual	char *	TimeOfDay();
virtual int32	CcDecode(uchar *inbuf, uchar *c0, uchar *c1);
virtual bool	CcParse(char c1, char c2);
virtual bool	XdsParse(char c1, char c2);
virtual void	PrintBuf(uchar *buf, uchar *buf2);

private:
	int32		mode;
	char		line[2][16][40];
	int32		bank,display_bank;
	int32		current_line;
	int32		rollup_start, rollup_end;
	
	char		*cc_info;
	char		cc_temp[40];
	char		lastc1;

	char		network_name[40];
	char		call_letters[40];
	char		program_name[40];
	char		time_of_day[40];
	
	char		*xds_info;
	char		xds_temp[40];
	
	uint32		vbi_samples;


};

#endif
