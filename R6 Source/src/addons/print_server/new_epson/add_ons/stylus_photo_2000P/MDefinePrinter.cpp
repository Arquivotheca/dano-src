#include <stdio.h>
#include <string.h>

#include <DataIO.h>
#include "MDefinePrinter.h"
#include "MColorProcess.h"

const char *gName = "Epson Stylus Photo 2000P";
const tPrinterInfo gPrinterInfo =
{
	gName,						// Name
	ID_STYLUS_PHOTO_2000P,		// ID
	8,							// DefaultPage
	48,							// Nb Black Nozzles
	48,							// Nb Color Nozzles
	120,						// DPI Black Nozzles
	120,						// DPI Color Nozzles
	0,							// -- not used
	0,							// -- not used
	256*1024,					// Internal Input buffer size
	5,							// Epson Remote level
	B_SOFTWARE_CONTROL,			// Flow control
	SYNCHRONOUS_SERIAL_MODE,	// Serial Mode
	0							// Size of the structure
};

const tPage gPage[] = {
		{ "User Defined",			42, 2976, 42, 42, 4800, 198, M_PLAIN_FORMAT },
		{ "Legal",					42, 2976, 42, 42, 4800, 198, M_PLAIN_FORMAT },
		{ "Letter",					42, 2976, 42, 42, 3720, 198, M_PLAIN_FORMAT },
		{ "Letter (landscape)",		42, 3870, 42, 42, 2815, 198, M_PLAIN_FORMAT },
		{ "Executive",				42, 2526, 42, 42, 3540, 198, M_PLAIN_FORMAT },
		{ "Half-Letter",			42, 1896, 42, 42, 2820, 198, M_PLAIN_FORMAT },
		{ "A3+",					42, 4578, 42, 42, 6605, 198, M_PLAIN_FORMAT },		
		{ "A3",						42, 4122, 42, 42, 5712, 198, M_PLAIN_FORMAT },		
		{ "A4",						42, 2892, 42, 42, 3969, 198, M_PLAIN_FORMAT },
		{ "A4 (landscape)",			42, 4116, 42, 42, 2730, 198, M_PLAIN_FORMAT },
		{ "A5",						42, 2020, 42, 42, 2736, 198, M_PLAIN_FORMAT },
		{ "A6",						42, 1404, 42, 42, 1864, 198, M_PLAIN_FORMAT },
		{ "B",						42, 3876, 42, 42, 5885, 198, M_PLAIN_FORMAT },
		{ "B4",						42, 3558, 42, 42, 4919, 198, M_PLAIN_FORMAT },
		{ "B5",						42, 2495, 42, 42, 3402, 198, M_PLAIN_FORMAT },
		{ "#10 Envelope",			42, 2976, 42, 42, 1245, 198, M_ENVELOPE_FROMAT },
		{ "DL Envelope",			42, 2976, 42, 42, 1319, 198, M_ENVELOPE_FROMAT },
		{ "C6 Envelope",			42, 2212, 42, 42, 1375, 198, M_ENVELOPE_FROMAT },
		{ "Envelope 132 x 220",		42, 2976, 42, 42, 1631, 198, M_ENVELOPE_FROMAT },
		{ "Index card 5x8",			42, 1716, 42, 42, 2640, 198, M_INDEX_CARD_FROMAT },
		{ "Index card 8x10",		42, 2796, 42, 42, 3360, 198, M_INDEX_CARD_FROMAT },
		{ "Photo Paper 4x6",		42, 1525, 42, 42, 2245, 198, M_PLAIN_FORMAT },
		{ "Panoramic Photo Paper",	42, 2892, 42, 120, 8100, 198, M_PLAIN_FORMAT }
				};

const tPage gPageLimits[2] = {
		{ "_min",					42, 1356, 42, 42,  1199, 199, M_UNKNOWN_FORMAT },
		{ "_max",					42, 4578, 42, 42, 15600, 199, M_UNKNOWN_FORMAT }
						};

const tPrinterRes gRes[] = {
		{ "Normal (720 DPI)",	 720, 720 },
		{ "Fine (1440 DPI)",	1440, 720 },
		{ "Photo (1440 DPI)",	1440, 720 },
		{ "Photo (2880 DPI)",	2880, 720 }
					};

const tPrinterPaper gPaper[] = {
		{ "Plain paper" 						},
		{ "Archival Matte paper"				},
		{ "Semi Gloss Photo paper"				},
		{ "Photo paper"							}
								};

const struct
{
	uint8 feed;
	uint8 gap;
} paperFeedValue[sizeof(gPage)/sizeof(tPage)][sizeof(gPaper)/sizeof(tPrinterPaper)] = {
	{ {0xB,0},{10,1},{8,1},{8,1} },	// userdef
	{ {0xB,0},{10,1},{8,1},{8,1} },	// legal
	{ {0xB,0},{10,1},{8,1},{8,1} },	// letter
	{ {0xB,0},{10,1},{8,1},{8,1} },	// letter landscape
	{ {0xB,0},{10,1},{8,1},{8,1} },	// executive
	{ {0xB,0},{10,1},{8,1},{8,1} },	// half letter
	{ {0xB,0},{10,1},{8,1},{8,1} },	// a3+
	{ {0xB,0},{10,1},{8,1},{8,1} },	// a3
	{ {0xB,0},{10,1},{8,1},{8,1} },	// a4
	{ {0xB,0},{10,1},{8,1},{8,1} },	// a4 landscape
	{ {0xB,0},{10,1},{8,1},{8,1} },	// a5
	{ {0xB,0},{10,1},{8,1},{8,1} },	// a6
	{ {0xB,0},{10,1},{8,1},{8,1} },	// b
	{ {0xB,0},{10,1},{8,1},{8,1} },	// b4
	{ {0xB,0},{10,1},{8,1},{8,1} },	// b5
	{ {0xB,2},{10,2},{8,2},{8,2} },	// #10 envelope
	{ {0xB,2},{10,2},{8,2},{8,2} },	// DL
	{ {0xB,2},{10,2},{8,2},{8,2} },	// C6
	{ {0xB,2},{10,2},{8,2},{8,2} },	// 132x220
	{ {0xB,1},{10,1},{8,1},{8,1} },	// index 5x8
	{ {0xB,1},{10,1},{8,1},{8,1} },	// index 8x10
	{ {0xB,0},{10,1},{8,1},{8,1} },	// photo paper
	{ {0xB,0},{10,1},{8,1},{8,1} },	// panoramique
};

const tColorMode gColorMode[] = {	{"Color",			6 },
									{"Black & White",	1 }		};

const tWeaveMode gWeaveMode[] = {	{"MicroWeave OFF", 	0 },
									{"MicroWeave ON",	1 },
									{"MicroWeave Super",0 }		};

const tSpeedMode gSpeedMode[] = {	{"Speed Bidir.",	0 },
									{"Speed Normal",	1 }		};

const tMicroDot gMicroDot[] =	{	{"Default",			0 },
									{"VD-4pl",			0x10 }	};

// resolution, paper -> icone(3 max, niveau de qualite), microdot, microweave, speed (low=1)
const int32 gRes2Paper_default = 0;
const tRes2Paper gRes2Paper[] =	{
									{0, 0, 0,	0, 2, 0x0000 },	// 720x720 plain
									{1, 1, 1,	0, 2, 0x0020 },	// 1440x720 archival
									{2, 1, 2,	1, 2, 0x0040 },	// 1440x720 archival
									{1, 2, 1,	0, 2, 0x0021 },	// 1440x720 semigloss
									{2, 2, 2,	1, 2, 0x0041 },	// 1440x720 semigloss
									{3, 2, 2,	1, 2, 0x0081 },	// 2880x720 semigloss
									{1, 3, 1,	0, 2, 0x0021 },	// 1440x720 glossy
									{2, 3, 2,	1, 2, 0x0041 },	// 1440x720 glossy
									{3, 3, 2,	1, 2, 0x0081 },	// 2880x720 glossy
								};

const int gNbPages		= sizeof(gPage)/sizeof(tPage);
const int gNbRes		= sizeof(gRes)/sizeof(tPrinterRes);
const int gNbPaper		= sizeof(gPaper)/sizeof(tPrinterPaper);
const int gNbColorMode	= sizeof(gColorMode)/sizeof(tColorMode);
const int gNbWeaveMode	= sizeof(gWeaveMode)/sizeof(tWeaveMode);
const int gNbSpeedMode	= sizeof(gSpeedMode)/sizeof(tSpeedMode);
const int gNbMicroDot	= sizeof(gMicroDot)/sizeof(tMicroDot);
const int gNbAllowed	= sizeof(gRes2Paper)/sizeof(tRes2Paper);

// ---------------------------------------------------------------

class MPrinterDriver : public MDefinePrinter
{
public:
		MPrinterDriver();
	virtual	status_t	ioctl			(uint32 cmd, void *arg1);
	virtual MColorProcess* instantiate_MColorProcess(tPrinterDef *pDriver);
};


MDefinePrinter *instantiate_MDefinePrinter()
{
	return new MPrinterDriver();
}


MPrinterDriver::MPrinterDriver()
{
	fPrinterInfo = gPrinterInfo;
	fAttributes = EP_SUPPORTS_USB | EP_SUPPORTS_EXPANDED | EP_IS_PHOTO;

	fPage = gPage;
	fPageLimits = gPageLimits;
	fRes = gRes;
	fPaper = gPaper;
	fColorMode = gColorMode;
	fWeaveMode = gWeaveMode;
	fSpeedMode = gSpeedMode;
	fMicroDot = gMicroDot;
	fRes2Paper = gRes2Paper;

	fNbPages = gNbPages;
	fNbRes = gNbRes;
	fNbPaper = gNbPaper;
	fNbColorMode = gNbColorMode;
	fNbWeaveMode = gNbWeaveMode;
	fNbSpeedMode = gNbSpeedMode;
	fNbMicroDot = gNbMicroDot;
	fNbAllowed = gNbAllowed;
	fRes2Paper_default = gRes2Paper_default;
}

MColorProcess *MPrinterDriver::instantiate_MColorProcess(tPrinterDef *printer_def)
{
	return new MColorProcess(printer_def);
}

status_t MPrinterDriver::ioctl(uint32 cmd, void *arg1)
{
	switch (cmd)
	{
		case IOCTL_GET_PAGE_TYPE:
		{
			int32 index = *(int32 *)arg1;
			if ((index < 0) || (index >= gNbPages))
				return B_ERROR;
			*(uint32 *)arg1 = gPage[index].type;
			return B_OK;
		}
		case IOCTL_PAPER_FEED_SEQUENCE:
		{
			tPaperFeedSequence& paperFeed = *((tPaperFeedSequence *)arg1);
			uint8 cmd0[7] = {'S','N',3,0, 0,0,0};
			uint8 cmd1[7] = {'S','N',3,0, 0,1,0};
			uint8 cmd2[7] = {'S','N',3,0, 0,2,0};
			cmd0[6] = paperFeedValue[paperFeed.page][paperFeed.paper].feed;
			cmd1[6] = paperFeedValue[paperFeed.page][paperFeed.paper].gap;
			cmd2[6] = (paperFeed.res == 0) ? (0) : (1);
			if (paperFeed.port->Write(cmd0, 7) != 7)	return B_ERROR;
			if (paperFeed.port->Write(cmd1, 7) != 7)	return B_ERROR;
			if (paperFeed.port->Write(cmd2, 7) != 7)	return B_ERROR;
			return B_OK;
		}
	}
	return MDefinePrinter::ioctl(cmd, arg1);
}

