#include <stdio.h>
#include <string.h>

#include <DataIO.h>
#include "MDefinePrinter.h"
#include "MColorProcess.h"

const char *gName = "EPSON Stylus Pro XL";
const tPrinterInfo gPrinterInfo =
{
	gName,						// Name
	ID_STYLUS_PRO_XL,			// ID
	4,							// DefaultPage
	64,							// Nb Black Nozzles
	16,							// Nb Color Nozzles
	90,							// DPI Black Nozzles  *
	90,							// DPI Color Nozzles  *
	0,							// Black delta nozzle *
	0,							// Color delta nozzle *
	64*1024,					// Internal Input buffer size
	0,							// Epson Remote level
	B_SOFTWARE_CONTROL,			// Flow control
	ASYNCHRONOUS_SERIAL_MODE,	// Serial Mode
	0							// Soze of the structure
};

const tPage gPage[] = {
		{ "User Defined",			43, 2975, 43, 43, 4800, 198, M_PLAIN_FORMAT },
		{ "Legal",					43, 2975, 43, 43, 4800, 198, M_PLAIN_FORMAT },
		{ "Letter",					43, 2975, 43, 43, 3720, 198, M_PLAIN_FORMAT },
		{ "Letter (landscape)",		43, 3870, 43, 43, 2815, 198, M_PLAIN_FORMAT },
		{ "A4",						43, 2891, 43, 43, 3969, 198, M_PLAIN_FORMAT },
		{ "A4 (landscape)",			43, 4115, 43, 43, 2730, 198, M_PLAIN_FORMAT },
		{ "Executive",				43, 2525, 43, 43, 3540, 198, M_PLAIN_FORMAT },
		{ "Statement",				43, 1895, 43, 43, 2820, 198, M_PLAIN_FORMAT },
		{ "A6 Index",				43, 1403, 43, 43, 1857, 198, M_INDEX_CARD_FROMAT },
		{ "#10 Envelope",			43, 3335, 43, 43, 1245, 198, M_ENVELOPE_FROMAT },
		{ "C5 Envelope",			43, 3159, 43, 43, 2056, 198, M_ENVELOPE_FROMAT },
		{ "DL Envelope",			43, 3032, 43, 43, 1319, 198, M_ENVELOPE_FROMAT },
		{ "Epson Photo Paper",		43, 1523, 43, 43, 2243, 198, M_POSTCARD_FROMAT },
		{ "A3",						43, 4124, 43, 43, 5712, 198, M_PLAIN_FORMAT },
		{ "Super A3/B",				43, 4577, 43, 43, 6605, 198, M_PLAIN_FORMAT },
		{ "US-B",					43, 3875, 43, 43, 5885, 198, M_PLAIN_FORMAT }
				};

const tPage gPageLimits[2] = {
		{ "_min",					43, 1356, 43, 43,  1199, 198, M_UNKNOWN_FORMAT },
		{ "_max",					43, 4577, 43, 43, 15601, 198, M_UNKNOWN_FORMAT }
						};

const tPrinterRes gRes[] = {
		{ "Draft (180 Dpi)",		180, 180 },
		{ "Normal (360 Dpi)",		360, 360 },
		{ "Super (720 Dpi)",		720, 720 },
		{ "High (720 Dpi)",			720, 360 }	};

const tPrinterPaper gPaper[] = {
		{ "Normal paper" 						},
		{ "Coated paper 360 dpi"				},
		{ "Coated paper 720 dpi"				},
		{ "Transparency paper"					},
		{ "EPSON Glossy paper, photo quality"	},
		{ "EPSON Glossy film, photo quality"	}	};

const tColorMode gColorMode[] = {	{"Color (CMYK)",	4 },
									{"Black & White",	1 }		};

const tWeaveMode gWeaveMode[] = {	{"MicroWeave OFF", 	0 },
									{"MicroWeave ON",	1 },
									{"MicroWeave Super",0 }		};

const tSpeedMode gSpeedMode[] = {	{"Speed Bidir.",	0 },
									{"Speed Normal",	1 }		};

const tMicroDot gMicroDot[] =	{	{"Normal dot",		2 },
									{"Small dot",		1 }		};

// resolution, paper -> icone(3 max, niveau de qualite), microdot, microweave, speed (low=1)
const int32 gRes2Paper_default = 1;
const tRes2Paper gRes2Paper[] =	{
										{0, 0, 0,	0, 0, 0 },	// 180x180 normal : normal
										{1, 0, 1,	0, 0, 0 },	// 360x360 normal : normal
										{3, 0, 2,	1, 2, 1 },	//--- 720x360 normal : small
										{1, 1, 1,	0, 1, 1 },	// 360x360 coated : normal
										{2, 2, 2,	1, 1, 1 },	// 720x720 coated : small
										{1, 3, 1,	0, 1, 1 },	// 360x360 transparency : normal
										{2, 4, 2,	1, 1, 1 },  // 720x720 glossy : small
										{3, 5, 2,	1, 2, 1 }	//--- 720x360 film : small
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


MPrinterDriver *instantiate_MDefinePrinter()
{
	return new MPrinterDriver();
}



MPrinterDriver::MPrinterDriver()
{
	fPrinterInfo = gPrinterInfo;
	fAttributes = EP_SUPPORTS_USB;

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
		return B_OK;
	}
	return MDefinePrinter::ioctl(cmd, arg1);
}

