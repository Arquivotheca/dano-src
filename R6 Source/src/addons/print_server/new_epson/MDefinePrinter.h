#ifndef MDEFINE_PRINTER_H
#define MDEFINE_PRINTER_H

#include <SupportDefs.h>
#include <SerialPort.h>

#include "DriverTypes.h"
#include "EpsonPrintingSystem.h"

class MDefinePrinter;
class MColorProcess;
class BDataIO;

extern "C" _EXPORT MDefinePrinter *instantiate_MDefinePrinter();

enum
{
	ID_STYLUS_COLOR			= 10,
	ID_STYLUS_PRO			= 20,
	ID_STYLUS_PRO_XL		= 21,
	ID_STYLUS_PRO_XL_PLUS	= 22,
	ID_STYLUS_COLOR_II		= 23,
	ID_STYLUS_COLOR_IIs		= 24,
	ID_STYLUS_400			= 400,
	ID_STYLUS_440			= 440,
	ID_STYLUS_500			= 500,
	ID_STYLUS_600			= 600,
	ID_STYLUS_640			= 640,
	ID_STYLUS_670			= 670,
	ID_STYLUS_740			= 740,
	ID_STYLUS_800			= 800,
	ID_STYLUS_850			= ID_STYLUS_800,
	ID_STYLUS_880			= 880,
	ID_STYLUS_900			= 900,
	ID_STYLUS_980			= ID_STYLUS_900,
	ID_STYLUS_1500			= 1500,
	ID_STYLUS_1520			= 1520,
	ID_STYLUS_3000			= 3000,
	ID_STYLUS_PHOTO			= 699,
	ID_STYLUS_PHOTO_700		= 700,
	ID_STYLUS_PHOTO_750		= 750,
	ID_STYLUS_PHOTO_870		= 870,
	ID_STYLUS_PHOTO_780		= 780,
	ID_STYLUS_PHOTO_790		= 790,
	ID_STYLUS_PHOTO_1270	= ID_STYLUS_PHOTO_870,
	ID_STYLUS_PHOTO_2000P	= 2000,
	ID_STYLUS_760			= 760,
	ID_STYLUS_777			= 777,
	ID_STYLUS_1160			= ID_STYLUS_760,
	ID_STYLUS_PHOTO_EX		= 701,
	ID_STYLUS_PHOTO_PM770C	= ID_STYLUS_PHOTO_750
};

enum
{
	IOCTL_GET_EPSON_REMOTE_LEVEL	= 0,
	IOCTL_GET_FLOW_CONTROL,
	IOCTL_GET_SERIAL_MODE,
	IOCTL_GET_PAGE_TYPE,
	IOCTL_SUPPORT_RLE_SWITCH,
	IOCTL_PARSE_STATUS,
	IOCTL_GET_EXPANDED_SUPPORT,
	IOCTL_PAPER_FEED_SEQUENCE
};

enum
{
	ASYNCHRONOUS_SERIAL_MODE = 0x01,
	SYNCHRONOUS_SERIAL_MODE = 0x2
};

enum
{
	M_UNKNOWN_FORMAT = -1,
	M_PLAIN_FORMAT = 0,
	M_ENVELOPE_FORMAT,
	M_POSTCARD_FORMAT,
	M_INDEX_CARD_FORMAT,

	// Hack because of an error
	M_ENVELOPE_FROMAT = M_ENVELOPE_FORMAT,
	M_POSTCARD_FROMAT = M_POSTCARD_FORMAT,
	M_INDEX_CARD_FROMAT = M_INDEX_CARD_FORMAT
	// end hack
};

typedef struct
{
	int		res_index;
	int		paper_index;
	int		icone;
	int		microdot_index;
	int		microweave_index;
	int		speed_index;
} tRes2Paper;


typedef struct
{
	const char 	*gName;
	int		gID;
	int32	gDefaultPage;
	uint32	gNbBlackNozzle;
	uint32	gNbColorNozzle;
	uint32	gBlackNozzleDPI;
	uint32	gColorNozzleDPI;
	uint32	gBlackDeltaNozzle;
	uint32	gColorDeltaNozzle;
	uint32	gMemory;
	uint32	gEpsonRemoteLevel;
	uint32	gSerialFlowControl;
	uint32	gSerialSyncMode;
	uint32	_size;
} tPrinterInfo;

typedef struct
{
	int32			res;
	int32			paper;
	int32			paperformat;
	int32			color_mode;
	int32			weave_mode;
	int32			speed_mode;
	int32			microdot;
	bool			expanded;
	uint16			printer_width;
	uint16			printer_height;
	uint16			printable_top;
	uint16			printable_bottom;
	int				hslices;
	int				soft_microweave;
	tPrinterInfo	printer_info;
} tPrinterDef;

struct tPaperFeedSequence
{
	tPaperFeedSequence(BDataIO *prt, uint8 r, uint8 p, uint8 pt)
		: port(prt), res(r), page(p), paper(pt) { }
	BDataIO *port;
	uint8 res;
	uint8 page;
	uint8 paper;
};

struct tPage
{
	char 	*name;
	unsigned int	lm;
	unsigned int	pw;
	unsigned int	rm;
	unsigned int	tm;
	unsigned int	ph;
	unsigned int	bm;
	int				type;
	uint32	reserved[4];
};

class MDefinePrinter
{
public:
	enum
	{
		M_DEFAULT = -1
	};

	MDefinePrinter();
	virtual ~MDefinePrinter(void);
	
			const char	*Name			(void);
			int			ID				(void);
			int32		DefaultPage		(void);
			uint32		NbBlackNozzle	(void);
			uint32		NbColorNozzle	(void);
			uint32		Memory			(void);
			status_t	GetPrinterInfo	(tPrinterInfo *info);
			bool 		IsEpsonModule	();

	virtual	status_t	PageLimits		(tPageFormat *min,	tPageFormat *max);
	virtual status_t	FindRes2Paper	(const tRes2Paper **res, int32 index);
	virtual	status_t	Page			(tPageFormat *page,				int32 index);
	virtual status_t	Res				(const tPrinterRes **res,		int32 index);
	virtual	status_t	Paper			(const tPrinterPaper **paper,	int32 index);
	virtual	status_t	ColorMode		(const tColorMode **color,		int32 index);
	virtual	status_t	WeaveMode		(const tWeaveMode **weave,		int32 index);
	virtual	status_t	SpeedMode		(const tSpeedMode **speed,		int32 index);
	virtual	status_t	DotSize			(const tMicroDot **dot,			int32 index);
	virtual uint8		GetPaperFeedValue(int res, int paper_type, int paper_format);
	virtual	status_t	ioctl			(uint32 cmd, void *arg1);

	virtual MColorProcess* instantiate_MColorProcess(tPrinterDef *pDriver);

	// Epson module support
	virtual short sSEInit(tagSE_INIT_PARAM *param);
	virtual short sSEOut(tagSE_OUT_PARAM *param);
	virtual short sSEEnd(tagSE_END_PARAM *param);

	
protected:
	enum
	{
		EP_SUPPORTS_USB			= 0x00000001,
		EP_IS_PHOTO				= 0x00000002,
		EP_IS_EPSON_MODULE		= 0x00000004,
		EP_SUPPORTS_EXPANDED	= 0x00000008,
		EP_SUPPORTS_MULTIDOT	= 0x00000010,
		EP_SMART_IC				= 0x00000020,
	};
	
	uint32			fAttributes;
	tPrinterInfo	fPrinterInfo;
	const tPage			*fPage;
	const tPage			*fPageLimits;
	const tPrinterRes 	*fRes;
	const tPrinterPaper *fPaper;
	const tColorMode	*fColorMode;
	const tWeaveMode	*fWeaveMode;
	const tSpeedMode	*fSpeedMode;
	const tMicroDot		*fMicroDot;
	const tRes2Paper	*fRes2Paper;

	int fNbPages;
	int fNbRes;
	int fNbPaper;
	int fNbColorMode;
	int fNbWeaveMode;
	int fNbSpeedMode;
	int fNbMicroDot;
	int fNbAllowed;
	int fRes2Paper_default;
	bigtime_t fOpStartTime;
	
	uint32	reserved[14];
};

#endif

