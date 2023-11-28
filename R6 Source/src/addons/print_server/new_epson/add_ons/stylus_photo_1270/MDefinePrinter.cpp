#include <stdio.h>
#include <string.h>

#include <DataIO.h>
#include "MDefinePrinter.h"
#include "MColorProcess.h"

const char *gName = "Epson Stylus Photo 1270";
const tPrinterInfo gPrinterInfo =
{
	gName,						// Name
	ID_STYLUS_PHOTO_1270,		// ID
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
		{ "Postcard",				42, 1333, 42, 42, 1857, 198, M_POSTCARD_FROMAT },
		{ "Return Postcard",		42, 2750, 42, 42, 1857, 198, M_POSTCARD_FROMAT },
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
		{ "Normal (360 Dpi)",		360, 360 },
		{ "Fine (720 Dpi)",			360, 720 },
		{ "Photo (720 Dpi)",		720, 720 },
		{ "Photo (1440 Dpi)",		1440, 720 },
					};

const tPrinterPaper gPaper[] = {
		{ "Plain paper" 						},
		{ "360 dpi Ink Jet Paper"				},
		{ "Photo Quality Ink jet paper"			},
		{ "Matte Paper - Heavyweight"			},
		{ "Photo Paper"							},
		{ "Premium Glossy Photo Paper"			},
		{ "Photo Quality Glossy film"			},
		{ "Transparencies"						},
								};

const struct
{
	uint8 feed;
	uint8 gap;
} paperFeedValue[sizeof(gPage)/sizeof(tPage)][sizeof(gPaper)/sizeof(tPrinterPaper)] = {
	{ {0,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// userdef
	{ {0,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// legal
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// letter
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// letter landscape
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// executive
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// half letter
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// a3+
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// a3
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// a4
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// a4 landscape
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// a5
	{ {2,1},{2,1},{2,1},{6,1},{8,1},{8,1},{3,1},{3,1} },	// a6
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// B
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// B4
	{ {1,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// B5
	{ {4,2},{4,2},{4,2},{6,2},{8,2},{8,2},{3,2},{3,2} },	// #10 envelope
	{ {4,2},{4,2},{4,2},{6,2},{8,2},{8,2},{3,2},{3,2} },	// DL
	{ {4,2},{4,2},{4,2},{6,2},{8,2},{8,2},{3,2},{3,2} },	// C6
	{ {4,2},{4,2},{4,2},{6,2},{8,2},{8,2},{3,2},{3,2} },	// envelope 132x220
	{ {2,1},{2,1},{2,1},{6,1},{8,1},{8,1},{3,1},{3,1} },	// postcard
	{ {2,1},{2,1},{2,1},{6,1},{8,1},{8,1},{3,1},{3,1} },	// return postcard
	{ {2,1},{2,1},{2,1},{6,1},{8,1},{8,1},{3,1},{3,1} },	// index 5x8
	{ {2,1},{2,1},{2,1},{6,1},{8,1},{8,1},{3,1},{3,1} },	// index 8x10
	{ {0,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// photo paper
	{ {0,0},{7,0},{7,0},{6,1},{8,1},{8,1},{3,0},{3,0} },	// panoramique
};

const tColorMode gColorMode[] = {	{"Color",			6 },
									{"Black & White",	1 }		};

const tWeaveMode gWeaveMode[] = {	{"MicroWeave OFF", 	0 },
									{"MicroWeave ON",	1 },
									{"MicroWeave Super",0 }		};

const tSpeedMode gSpeedMode[] = {	{"Speed Bidir.",	0 },
									{"Speed Normal",	1 }		};

const tMicroDot gMicroDot[] =	{	{"Default",			0 },
									{"VD2",				0x11 },
									{"VD1",				0x12 },
									{"VD0",				0x10 }
								};

// resolution, paper -> icone(3 max, niveau de qualite), microdot, microweave, speed (low=1)
const int32 gRes2Paper_default = 1;
const tRes2Paper gRes2Paper[] =	{
									{0, 0, 0,	2, 2, 0x0000 },	// 360x360 plain
									{2, 0, 1,	1, 2, 0x0021 },	// 720x720 plain
									{3, 0, 2,	3, 2, 0x0041 },	// 1440x720 plain
									{0, 1, 0,	2, 2, 0x0000 },	// 360x360 inkjet 360
									{1, 2, 1,	2, 2, 0x0000 },	// 360x720 inkjet
									{2, 2, 2,	1, 2, 0x0021 },	// 720x720 inkjet
									{3, 2, 2,	3, 2, 0x0041 },	// 1440x720 inkjet
									{1, 3, 1,	2, 2, 0x0001 },	// 360x720 matte
									{2, 3, 2,	1, 2, 0x0021 },	// 720x720 matte
									{3, 3, 2,	3, 2, 0x0041 },	// 1440x720 matte
									{1, 4, 1,	2, 2, 0x0001 },	// 360x720 photo
									{2, 4, 2,	1, 2, 0x0021 },	// 720x720 photo
									{3, 4, 2,	3, 2, 0x0041 },	// 1440x720 photo
									{1, 5, 1,	2, 2, 0x0001 },	// 360x720 premium
									{2, 5, 2,	1, 2, 0x0021 },	// 720x720 premium
									{3, 5, 2,	3, 2, 0x0041 },	// 1440x720 premium
									{2, 6, 2,	1, 2, 0x0021 },	// 720x720 film
									{3, 6, 2,	3, 2, 0x0041 },	// 1440x720 film
									{0, 7, 0,	2, 2, 0x0021 },	// 360x360 transparency
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

