/*
	
	Captions.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include "Captions.h"

//-----------------------------------------------------------------

ClosedCaption::ClosedCaption(uint32 num_samples)
{
	vbi_samples = num_samples;
	Reset();
}

//-----------------------------------------------------------------

ClosedCaption::~ClosedCaption()
{

}

//-----------------------------------------------------------------

void
ClosedCaption::Reset()
{
	int		i;

	mode = ROLLUP2;
	current_line = rollup_start = rollup_end = 15;	
	for (i=0; i < 16; i++)
	{
		line[0][i][0] = 0;
		line[1][i][0] = 0;
	}
	
	bank = 0;
	display_bank = 0;
	
	cc_info		= 0;
	cc_temp[0]	= 0;
	lastc1		= 0;
	
	call_letters[0]	= 0;
	network_name[0]	= 0;
	program_name[0]	= 0;
	time_of_day[0]	= 0;

	xds_info		= 0;
	xds_temp[0]		= 0;
}

//-----------------------------------------------------------------

int32
ClosedCaption::Mode()
{
	return mode;
}

//-----------------------------------------------------------------

int32
ClosedCaption::Bank()
{
	return bank;
}

//-----------------------------------------------------------------

int32
ClosedCaption::DisplayBank()
{
	return display_bank;
}

//-----------------------------------------------------------------

int32
ClosedCaption::RollupStart()
{
	return rollup_start;
}

//-----------------------------------------------------------------

int32
ClosedCaption::RollupEnd()
{
	return rollup_end;
}

//-----------------------------------------------------------------

int32
ClosedCaption::CurrentLine()
{
	return current_line;
}

//-----------------------------------------------------------------

char *
ClosedCaption::Line(int32 bank, int32 lineno)
{
	return line[bank][lineno];
}

//-----------------------------------------------------------------

char *
ClosedCaption::NetworkName()
{
	return network_name;
}

//-----------------------------------------------------------------

char *
ClosedCaption::CallLetters()
{
	return call_letters;
}

//-----------------------------------------------------------------

char *
ClosedCaption::ProgramName()
{
	return program_name;
}

//-----------------------------------------------------------------

char *
ClosedCaption::TimeOfDay()
{
	return time_of_day;
}

//-----------------------------------------------------------------

void
ClosedCaption::PrintBuf(uchar *buf, uchar *buf2)
{
	unsigned i,j;
	for (i = 0; i < vbi_samples/16; i++)
	{
		for (j = i*16; j < (i+1)*16; j++)
		{
			printf("%2x ",buf[j]);
		}
		printf("    ");
		for (j = i*16; j < (i+1)*16; j++)
		{
			printf("%2x ",buf2[j]);
		}
		printf("\n");
	}
	printf("\n");
}


//-----------------------------------------------------------------

int32
ClosedCaption::CcDecode(uchar *inbuf, uchar *c0, uchar *c1)
{

	#define CCD(x...)
	
	/* quick & dirty decoder */
	
	uint	i,j, k;
	uchar	min,max;
	uint	start,period,periodt;
	uint	clockedge[7];
	uint	clockedget[7];
	uchar	buf[B_PAL_VBI_SAMPLES + 32];
	char	parity0,parity1;

	*c0	= 0;
	*c1 = 1;
		
	min = 255;
	max = 0;
	
	#define NUM_TAPS	16
	#define STEP		2
	
	for (i =0; i < NUM_TAPS/STEP; i++)
	{
		buf[i] = 0x30;		
	}

	/* low pass filter, pek detect */				
	for (i = NUM_TAPS/STEP; i < vbi_samples/STEP; i++)
	{
		/*  clip negative glitches */
		if (inbuf[i*STEP] < 0x30)
			inbuf[i*STEP] = 0x30;
		
		k = 0;
		/* low pass filter */	
		for (j = i*STEP-NUM_TAPS; j < i*STEP; j++)
		{
			k = k + inbuf[j];
		}		
		buf[i] = (uchar) (k/NUM_TAPS);

		/* peak detect for AGC */
		if (buf[i] < min)
			min = (uint)buf[i];
		if (buf[i] > max)
			max = (uint)buf[i];
	}

	if ((max - min) < 0x30)
	{
		/* peak to peak amplitude too low for cc */
		CCD("no cc: min %d max %d\n",min,max);
		return NO_CAPTIONS;	
	}
	
	//normalize/quantize
	
	buf[0]=0;
	for (i = 1; i < vbi_samples/STEP; i++)
	{
		buf[i] = (uint32)((buf[i]-min) * 255)/(max-min);
		
		if ( buf[i] > (0x80 - buf[i-1]*0x8))
		{
			buf[i] = 1;
		}
		else
		{
			buf[i] = 0;
		}
	}
	
	/* put some non-0/non-1 values to terminate scans below */	
	for (i = vbi_samples/STEP; i < vbi_samples/STEP + 32; i++)
		buf[i] = 2;

	/* scan to first clock edge */	
	i = 8;	
	while (buf[i++] == 0) {;}

	if ( (i < 16/STEP) | (i > 128/STEP) )
	{
		/* reject lines without 0 level cc leadin format */		
		CCD("no cc %x\n",i);
		//PrintBuf(buf,inbuf);
		return NO_CAPTIONS;
	}

	/* scan for each clock edge  */	

	#define HALF_CYCLE	12
	
	for (j = 0; j < 6; j++)
	{
		clockedge[j] = i;
		while (buf[i++] == 1) {;}
		if ((i-clockedge[j]) < HALF_CYCLE/STEP)
		{
			CCD("clock %d H period too short\n", j);
			//PrintBuf(buf,inbuf);
			return NO_CAPTIONS;
		}
		clockedget[j] = i;
		while (buf[i++] == 0) {;}
		if ((i-clockedget[j]) < HALF_CYCLE/STEP)
		{
			CCD("clock %d H period too short\n", j);
			//PrintBuf(buf,inbuf);
			return NO_CAPTIONS;
		}
	}	
		
	clockedge[6] = i;
	while (buf[i++] == 1) {;}
	if ((i-clockedge[6]) < HALF_CYCLE/STEP)
		{
			CCD("clock 6 H period too short\n");
			//PrintBuf(buf,inbuf);
			return NO_CAPTIONS;
		}
	clockedget[6] = i;	
	
	period =	((clockedge[6] - clockedge[5]) +
				(clockedge[5] - clockedge[4]) +
				(clockedge[4] - clockedge[3]) +
				(clockedge[3] - clockedge[2]) +
				(clockedge[2] - clockedge[1]) +
				(clockedge[1] - clockedge[0]))/6;
				
	periodt =	((clockedget[6] - clockedget[5]) +
				(clockedget[5] - clockedget[4]) +
				(clockedget[4] - clockedget[3]) +
				(clockedget[3] - clockedget[2]) +
				(clockedget[2] - clockedget[1]) +
				(clockedget[1] - clockedget[0]))/6;

	period = (period + periodt + 1)/2;
	
	if (period > 64/STEP)
	{
		CCD("period too large %d %d %d %d\n",period,periodt,min,max);
		//PrintBuf(buf,inbuf);
		return NO_CAPTIONS;
	}
	
	start = i + period + period/2;
				
	if (buf[start] != 0)
	{
		CCD("missing start 0 bit 0x%x\n",start);
		//PrintBuf(buf,inbuf);
		return NO_CAPTIONS;
	}
	
	start = start + period;
	
	if (buf[start] != 1)
	{
		CCD("missing start 1 bit 0x%x\n",start);
		//PrintBuf(buf,inbuf);
		return NO_CAPTIONS;
	}
		
	*c0 = 	((buf[start + 1 * period])     | (buf[start + 2 * period] << 1) |
			(buf[start + 3 * period] << 2) | (buf[start + 4 * period] << 3) |
			(buf[start + 5 * period] << 4) | (buf[start + 6 * period] << 5) |
			(buf[start + 7 * period] << 6));
			
	parity0 = buf[start + 8 * period];

			
	*c1 = 	((buf[start +  9 * period])     | (buf[start + 10 * period] << 1) |
			(buf[start + 11 * period] << 2) | (buf[start + 12 * period] << 3) |
			(buf[start + 13 * period] << 4) | (buf[start + 14 * period] << 5) |
			(buf[start + 15 * period] << 6));

	parity1 = buf[start + 16 * period];
	
	/* for debug */
	for (j = 1; j < 17; j++)
		buf[start + j * period] += 0xf0;

	if (((buf[start + 1 * period] ^ buf[start + 2 * period] ^
		buf[start + 3 * period] ^ buf[start + 4 * period] ^
		buf[start + 5 * period] ^ buf[start + 6 * period] ^
		buf[start + 7 * period]) & 0x01) == parity0)
	{
		CCD(" C0 Parity error\n");
		//PrintBuf(buf,inbuf);
		return CAPTION_PARITY_ERROR;
	}

	if (((buf[start +  9 * period] ^ buf[start + 10 * period] ^
		buf[start + 11 * period] ^ buf[start + 12 * period] ^
		buf[start + 13 * period] ^ buf[start + 14 * period] ^
		buf[start + 15 * period]) & 0x01) == parity1)
	{
		CCD(" C1 Parity error\n");
		//PrintBuf(buf,inbuf);
		return CAPTION_PARITY_ERROR;
	}
	return CAPTION_OK;	
}

//-----------------------------------------------------------------

bool
ClosedCaption::CcParse(char c0, char c1)
{
	int		len,i;
	bool	display;	
	char	bit_bucket[40];

	display = false;
	
	if (cc_info == 0)
		cc_info = bit_bucket;
	
	len = strlen(cc_temp);

#define	CCP(x...)
	
	if (c0 < 0x20)
	{
		/* control codes are repeated twice */
		/* ignore if same as last time      */
		if (c1 == lastc1)
		{
			lastc1 = c1;
			return false;
		}
		switch(c0)
		{
			case 0:
				break;
			case 0x10:
			case 0x18:
				/* preamble address codes */
				switch (c1 & 0x60)
				{
					/* preamble address codes */
					case 0x40:
						CCP("preamble address row 11\n");
						strcpy(cc_info,cc_temp);
						current_line = 1;
						cc_info = line[bank][11];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = 
						;
						break;
					default:
						CCP("unimplemented preamble address display code\n");
						break;
				}
				break;
			case 0x11:
			case 0x19:
				/* preamble/midrow code */
				switch (c1 & 0x60)
				{
					/* preamble address codes */
					case 0x40:
						CCP("preamble address row 1\n");
						strcpy(cc_info,cc_temp);
						current_line = 1;
						cc_info = line[bank][1];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					case 0x60:
						CCP("preamble address row 2\n");
						strcpy(cc_info,cc_temp);
						current_line = 2;
						cc_info = line[bank][2];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					default:
					switch (c1)
					{
						/* midrow color atributes */
						case 0x20:
							CCP("white attribute\n");
							break;
						case 0x21:
							CCP("green attribute\n");
							break;
						case 0x22:
							CCP("blue attribute\n");
							break;
						case 0x23:
							CCP("cyan attribute\n");
							break;
						case 0x24:
							CCP("red attribute\n");
							break;
						case 0x25:
							CCP("yellow attribute\n");
							break;
						case 0x26:
							CCP("magenta attribute\n");
							break;
						case 0x27:
							CCP("italics attribute\n");
							break;
						/* special characters */
						case 0x30:
							CCP("spec char: cirle r\n");
							break;
						case 0x31:
							CCP("spec char: degree\n");
							break;
						case 0x32:
							CCP("spec char: one half\n");
							break;
						case 0x33:
							CCP("spec char: upside down ?\n");
							break;
						case 0x34:
							CCP("spec char: tm\n");
							break;
						case 0x35:
							CCP("spec char: cents\n");
							break;
						case 0x36:
							CCP("spec char: pound\n");
							break;
						case 0x37:
							CCP("spec char: note\n");
							break;
						case 0x38:
							CCP("spec char: a'\n");
							break;
						case 0x39:
							CCP("spec char: transparent space\n");
							break;
						case 0x3a:
							CCP("spec char: e'\n");
							break;
						case 0x3b:
							CCP("spec char: a^\n");
							break;
						case 0x3c:
							CCP("spec char: e^\n");
							break;
						case 0x3d:
							CCP("spec char: i^\n");
							break;
						case 0x3e:
							CCP("spec char: o^\n");
							break;
						case 0x3f:
							CCP("spec char: u^\n");
							break;
						default:
							CCP("unimplemented color attribute\n");
							break;
					}
				}
				break;
			case 0x12:
			case 0x1a:
				/* preamble address codes */
				switch (c1 & 0x60)
				{
					/* preamble address codes */
					case 0x40:
						CCP("preamble address row 3\n");
						strcpy(cc_info,cc_temp);
						current_line = 3;
						cc_info = line[bank][3];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					case 0x60:
						CCP("preamble address row 4\n");
						strcpy(cc_info,cc_temp);
						current_line = 4;
						cc_info = line[bank][4];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					default:
						CCP("unimplemented preamble address display code\n");
						break;
				}
				break;
			case 0x13:
			case 0x1b:
				/* preamble address codes */
				switch (c1 & 0x60)
				{
					/* preamble address codes */
					case 0x40:
						CCP("preamble address row 12\n");
						strcpy(cc_info,cc_temp);
						current_line = 12;
						cc_info = line[bank][12];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					case 0x60:
						CCP("preamble address row 13\n");
						strcpy(cc_info,cc_temp);
						current_line = 13;
						cc_info = line[bank][13];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					default:
						CCP("unimplemented preamble address display code\n");
						break;
				}
				break;
			case 0x14:
			case 0x1c:
				/* non display control byte -- misc code */
				switch (c1 & 0x60)
				{
					/* preamble address codes */
					case 0x40:
						CCP("preamble address row 14\n");
						strcpy(cc_info,cc_temp);
						current_line = 14;
						cc_info = line[bank][14];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					case 0x60:
						CCP("preamble address row 15\n");
						CCP("attribute = %x\n",c1 & 0x60);
						strcpy(cc_info,cc_temp);
						current_line = 15;
						cc_info = line[bank][15];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					default:
					switch (c1)
					{
						/* text placement control */
						case 0x20:
							CCP("resume caption loading\n");
							mode = POPUP;
							if (bank == display_bank)
								display_bank = bank ^ 1;
							break;
						case 0x21:
							CCP("backspace\n");
							cc_temp[--len] = 0;								
							break;
						case 0x22:
						case 0x23:
						case 0x24:
							CCP("reserved\n");
							break;
						case 0x25:
							CCP("roll up captions, 2 rows\n");
							mode = ROLLUP2;
							bank = display_bank = 0;
							rollup_start	= current_line - 1;
							rollup_end		= current_line;
							break;
						case 0x26:
							CCP("roll up captions, 3 rows\n");
							mode = ROLLUP3;
							bank = display_bank = 0;
							rollup_start	= current_line - 2;
							rollup_end		= current_line;
							break;
						case 0x27:
							CCP("roll up captions, 4 rows\n");
							mode = ROLLUP4;
							bank = display_bank = 0;
							rollup_start	= current_line - 3;
							rollup_end		= current_line;
							break;
						case 0x28:
							CCP("flash on captions\n");
							break;
						case 0x29:
							CCP("resume direct captioning\n");
							break;
						case 0x2a:
							CCP("text restart\n");
							mode = TEXT;
							bank = display_bank = 0;
							current_line = 1;
							rollup_start	= 1;
							rollup_end		= 15;
							cc_info = line[bank][1];
							cc_temp[0] = 0;
							for (i=0; i < 16; i++)
							{
								line[0][i][0] = 0;
								line[1][i][0] = 0;
							}
							break;
						case 0x2b:
							CCP("resume text display\n");
							mode = TEXT;
							break;
						case 0x2c:
							CCP("erase displayed memory\n");
							for (i=0; i < 16; i++) line[display_bank][i][0] = 0;
							display = true;
							break;
						case 0x2d:
							/* carriage return */
							CCP("CR\n");
							for (i = rollup_start; i <= rollup_end; i++)
							{
								strcpy(line[bank][i],line[bank][i+1]);	
							}
							strcpy(cc_info,cc_temp);
							display = true;
							break;
						case 0x2e:
							CCP("erase nondisplayed memory\n");
							for (i=0; i < 16; i++) line[bank][i][0] = 0;
							break;
						case 0x2f:
							CCP("end of caption\n");
							strcpy(cc_info,cc_temp);
							cc_info = bit_bucket;
							cc_temp[0] = 0;
							display_bank = bank;
							bank = bank ^ 1;
							display = true;
							break;
						default:
							CCP("unimplemented misc code %x\n",c1);
							break;
					}
				}
				break;
			case 0x15:
			case 0x1d:
				/* preamble address codes */
				switch (c1 & 0x60)
				{
					/* preamble address codes */
					case 0x40:
						CCP("preamble address row 5\n");
						strcpy(cc_info,cc_temp);
						current_line = 5;
						cc_info = line[bank][5];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					case 0x60:
						CCP("preamble address row 6\n");
						strcpy(cc_info,cc_temp);
						current_line = 6;
						cc_info = line[bank][6];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					default:
						CCP("unimplemented preamble address display code\n");
						break;
				}
				break;
			case 0x16:
			case 0x1e:
				/* preamble address codes */
				switch (c1 & 0x60)
				{
					/* preamble address codes */
					case 0x40:
						CCP("preamble address row 7\n");
						strcpy(cc_info,cc_temp);
						current_line = 7;
						cc_info = line[bank][7];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					case 0x60:
						CCP("preamble address row 8\n");
						strcpy(cc_info,cc_temp);
						current_line = 8;
						cc_info = line[bank][8];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					default:
						CCP("unimplemented preamble address display code\n");
						break;
				}
				break;
			case 0x17:
			case 0x1f:
				/* preamble address codes */
				switch (c1 & 0x60)
				{
					case 0x20:
						switch(c1 & 0x0f)
						{
							case 0x01:
								CCP("Tab 1\n");
								strcat(cc_temp," ");
								break;
							case 0x02:
								CCP("Tab 2\n");
								strcat(cc_temp,"  ");
								break;
							case 0x03:
								CCP("Tab 3\n");
								strcat(cc_temp,"   ");
								break;
							default:
								break;
						}
						break;
					/* preamble address codes */
					case 0x40:
						CCP("preamble address row 9\n");
						strcpy(cc_info,cc_temp);
						current_line = 9;
						cc_info = line[bank][9];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					case 0x60:
						CCP("preamble address row 10\n");
						strcpy(cc_info,cc_temp);
						current_line = 10;
						cc_info = line[bank][10];
						cc_temp[0] = 0;
						//if (mode != POPUP) display = true;
						break;
					default:
						CCP("unimplemented preamble address display code\n");
						break;
				}
				break;
			default:
				CCP("undefined char\n");
				break;
		}
	}
	else {
		if (len < 32)
		{
			cc_temp[len++] = c0;
			if (c1 != 0)
				cc_temp[len++] = c1;
			cc_temp[len] = 0;
		}		
	}
	
	lastc1 = c1;
	
	return display;
}

//-----------------------------------------------------------------

bool
ClosedCaption::XdsParse(char c0, char c1)
{

	int		len;
	bool	display;
	char	bit_bucket[40];

	display = false;
		
	if (xds_info == 0)
		xds_info = bit_bucket;
		
	len = strlen(xds_temp);

	if (c0 < 0x10)
	{
		switch(c0)
		{
			/* Current program start */
			case 0x01:
				switch(c1)
				{
					case 0x03:
						xds_info = program_name;
						xds_temp[0] = 0;
						break;	
				}
				break;
			/* Current program continue */
			case 0x02:
				break;
			/* Channel start */
			case 0x05:
				switch(c1)
				{
					case 0x01:
						xds_info = network_name;
						xds_temp[0] = 0;
						break;	
					case 0x02:
						xds_info = call_letters;
						xds_temp[0] = 0;
						break;	
				}
				break;
			/* Channel continue */
			case 0x06:
				break;
			/* Misc start */
			case 0x07:
				switch(c1)
				{
					case 0x01:
						xds_info = time_of_day;
						xds_temp[0] = 0;
						break;
				}
				break;
			/* end */
			case 0x0f:
				if ((xds_info != bit_bucket) & (strcmp(xds_info,xds_temp) != 0))
				{
					strcpy(xds_info,xds_temp);
					display = true;
				}
				xds_info = bit_bucket;
				xds_temp[0] = 0;				
				break;
		}
	}
	else
	{
		if (len < 32)
		{
			xds_temp[len++] = c0;
			if (c1 != 0)
				xds_temp[len++] = c1;
			xds_temp[len] = 0;
		}		
	}
	
	return display;
}

//-----------------------------------------------------------------

