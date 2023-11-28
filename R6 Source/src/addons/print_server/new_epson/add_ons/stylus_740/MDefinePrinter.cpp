#include <stdio.h>
#include <string.h>

#include <DataIO.h>
#include "MDefinePrinter.h"
#include "MColorProcess.h"

const char *gName = "Epson Stylus Color 740";
const tPrinterInfo gPrinterInfo =
{
	gName,						// Name
	ID_STYLUS_740,				// ID
	3,							// DefaultPage
	144,						// Nb Black Nozzles
	48,							// Nb Color Nozzles
	120,						// DPI Black Nozzles
	120,						// DPI Color Nozzles
	0,							// -- not used
	0,							// -- not used
	256*1024,					// Internal Input buffer size
	4,							// Epson Remote level
	B_SOFTWARE_CONTROL,			// Flow control
	SYNCHRONOUS_SERIAL_MODE,	// Serial Mode
	0							// Size of the structure
};

const tPage gPage[] = {
		{ "User Defined",			42, 2976, 42, 42, 3720, 199, M_PLAIN_FORMAT },
		{ "Letter",					42, 2976, 42, 42, 3720, 199, M_PLAIN_FORMAT },
		{ "Legal",					42, 2976, 42, 42, 4800, 199, M_PLAIN_FORMAT },
		{ "A4",						42, 2892, 42, 42, 3969, 199, M_PLAIN_FORMAT },
		{ "A5",						42, 2020, 42, 42, 2736, 199, M_PLAIN_FORMAT },
		{ "A6 Index",				42, 1404, 42, 42, 1864, 199, M_INDEX_CARD_FROMAT },
		{ "Executive",				42, 2526, 42, 42, 3540, 199, M_PLAIN_FORMAT	},
		{ "Half Letter",			42, 1896, 42, 42, 2820, 199, M_PLAIN_FORMAT },
		{ "Index Card 5 in x 8 in",	42, 1716, 42, 42, 2640, 199, M_INDEX_CARD_FROMAT },
		{ "Index Card 8 in x 10 in",42, 2796, 42, 42, 3360, 199, M_INDEX_CARD_FROMAT },
		{ "#10 Envelope",			42, 2976, 42, 42, 1245, 199, M_ENVELOPE_FROMAT },
		{ "DL Envelope",			42, 2976, 42, 42, 1319, 199, M_ENVELOPE_FROMAT },
		{ "C6 Envelope",			42, 2212, 42, 42, 1375, 199, M_ENVELOPE_FROMAT },
		{ "Epson Photo Paper",		42, 1523, 42, 42, 2243, 199, M_POSTCARD_FROMAT },
		{ "Panoramic Photo Paper",	42, 2892, 42, 120,8100, 199, M_PLAIN_FORMAT }
				};

const tPage gPageLimits[2] = {
		{ "_min",					42, 1356, 42, 42,  1199, 199, M_UNKNOWN_FORMAT },
		{ "_max",					42, 2976, 42, 42, 15600, 199, M_UNKNOWN_FORMAT }
						};

const tPrinterRes gRes[] = {
		{ "Normal (360 Dpi)",		360, 360 },
		{ "High (360 Dpi)",			720, 360 },
		{ "Super (720 Dpi)",		720, 720 },
		{ "Photo (1440 Dpi)",		1440, 720 }
					};

const tPrinterPaper gPaper[] = {
		{ "Plain paper" 						},	// Papier normal			: 720, 1440
		{ "Inkjet 360 DPI"						},	// inkjet 360				: 360 dpi
		{ "Photo Quality Ink jet paper"			},	// Papier 720				: 720, 1440
		{ "Photo Paper"							},	// Papier photo (glacé)		: 720, 1440
		{ "Photo Quality Glossy film"			},	// Film glacé				: 720, 1440
		{ "Transparency"						},	// Transparents				: 360 dpi
								};

const uint8 paperFeedValue[sizeof(gPage)/sizeof(tPage)][sizeof(gPaper)/sizeof(tPrinterPaper)] = {
	{ 1, 1, 1, 1, 3, 3 },	// userdef
	{ 1, 1, 1, 1, 3, 3 },	// letter
	{ 1, 1, 1, 1, 3, 3 },	// legal
	{ 1, 1, 1, 1, 3, 3 },	// a4
	{ 1, 1, 1, 1, 3, 3 },	// a5
	{ 1, 1, 1, 1, 3, 3 },	// a6
	{ 1, 1, 1, 1, 3, 3 },	// executive
	{ 1, 1, 1, 1, 3, 3 },	// half letter
	{ 2, 2, 2, 1, 3, 3 },	// index 5x8
	{ 2, 2, 2, 1, 3, 3 },	// index 8x10
	{ 4, 4, 4, 1, 3, 3 },	// #10 envelope
	{ 4, 4, 4, 1, 3, 3 },	// DL
	{ 4, 4, 4, 1, 3, 3 },	// C6
	{ 1, 1, 1, 1, 3, 3 },	// photo paper
	{ 1, 1, 1, 1, 3, 3 },	// panoramique
};

const tColorMode gColorMode[] = {	{"Color",			4 },
									{"Black & White",	1 }		};

const tWeaveMode gWeaveMode[] = {	{"MicroWeave OFF", 	0 },
									{"MicroWeave ON",	1 },
									{"MicroWeave Super",0 }		};

const tSpeedMode gSpeedMode[] = {	{"Speed Bidir.",	0 },
									{"Speed Normal",	1 }		};

const tMicroDot gMicroDot[] =	{	{"Micro dot",		0x01 },
									{"Single dot",		0x02 },
									{"Double dot",		0x03 },
									{"Quad dot",		0x04 },
									{"Multisize",		0x10 } 	};

// resolution, paper -> icone(3 max, niveau de qualite), microdot, microweave, slices | speed (low=1)
const int32 gRes2Paper_default = 1;
const tRes2Paper gRes2Paper[] =	{
									{0, 0, 0,	3, 0, 0x0000 },	// 360x360 plain 1
									{2, 0, 1,	2, 2, 0x0000 },	// 720x720 plain 1
									{2, 0, 2,	4, 2, 0x0021 },	// 720x720 plain 2
									{1, 1, 1,	2, 2, 0x0001 },	// 720x360 inkjet360 1
									{2, 2, 0,	2, 2, 0x0001 },	// 720x720 inkjet 1
									{2, 2, 1,	4, 2, 0x0021 },	// 720x720 inkjet 2
									{3, 2, 2,	4, 2, 0x0041 },	// 1440x720 inkjet 2
									{2, 3, 0,	2, 2, 0x0001 },	// 720x720 photo 1
									{2, 3, 1,	4, 2, 0x0021 },	// 720x720 photo 2
									{3, 3, 2,	4, 2, 0x0041 },	// 1440x720 photo 2
									{3, 4, 2,	4, 2, 0x0041 },	// 1440x720 glossy 2
									{0, 5, 2,	3, 2, 0x0001 },	// 360x360 transp
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
	fAttributes = EP_SUPPORTS_USB | EP_SUPPORTS_EXPANDED;

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
			cmd0[6] = paperFeedValue[paperFeed.page][paperFeed.paper];
			if (paperFeed.port->Write(cmd0, 7) != 7)
				return B_ERROR;
			return B_OK;
		}
	}
	return MDefinePrinter::ioctl(cmd, arg1);
}

