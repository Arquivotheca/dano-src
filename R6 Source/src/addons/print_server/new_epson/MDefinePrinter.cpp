#include <stdio.h>
#include <string.h>

#include <Roster.h>
#include <AppFileInfo.h>

#include "MDefinePrinter.h"


MDefinePrinter::MDefinePrinter()
{
	fOpStartTime = 0;
}

MDefinePrinter::~MDefinePrinter()
{
}

uint8 MDefinePrinter::GetPaperFeedValue(int res, int paper_type, int paper_format)
{
	return 0;
}

status_t MDefinePrinter::GetPrinterInfo	(tPrinterInfo *info)
{
	if (info->_size == sizeof(tPrinterInfo)) {
		*info = fPrinterInfo;
		return B_OK;
	}
	return B_BAD_VALUE;
}

const char *MDefinePrinter::Name(void)
{
	return fPrinterInfo.gName;
}

int MDefinePrinter::ID(void)
{
	return fPrinterInfo.gID;
}

int32 MDefinePrinter::DefaultPage(void)
{
	return fPrinterInfo.gDefaultPage;
}

uint32 MDefinePrinter::NbBlackNozzle(void)
{
	return fPrinterInfo.gNbBlackNozzle;
}

uint32 MDefinePrinter::NbColorNozzle(void)
{
	return fPrinterInfo.gNbColorNozzle;
}

uint32 MDefinePrinter::Memory(void)
{
	return fPrinterInfo.gMemory;
}

// -------------------------------------------------------------


status_t MDefinePrinter::Page(tPageFormat *page, int32 index)
{
	if ((index < 0) || (index >= fNbPages))
		return B_ERROR;	
	page->PageName			= fPage[index].name;
	page->left_margin 		= fPage[index].lm;
	page->printable_width 	= fPage[index].pw;
	page->right_margin 		= fPage[index].rm;
	page->top_margin 		= fPage[index].tm;
	page->printable_height 	= fPage[index].ph;
	page->bottom_margin 	= fPage[index].bm;
	return B_OK;
}

status_t MDefinePrinter::PageLimits(tPageFormat *min, tPageFormat *max)
{
	min->PageName			= fPageLimits[0].name;
	min->left_margin 		= fPageLimits[0].lm;
	min->printable_width 	= fPageLimits[0].pw;
	min->right_margin 		= fPageLimits[0].rm;
	min->top_margin 		= fPageLimits[0].tm;
	min->printable_height 	= fPageLimits[0].ph;
	min->bottom_margin 		= fPageLimits[0].bm;
	max->PageName			= fPageLimits[1].name;
	max->left_margin 		= fPageLimits[1].lm;
	max->printable_width 	= fPageLimits[1].pw;
	max->right_margin 		= fPageLimits[1].rm;
	max->top_margin 		= fPageLimits[1].tm;
	max->printable_height 	= fPageLimits[1].ph;
	max->bottom_margin 		= fPageLimits[1].bm;
	return B_OK;
}

status_t MDefinePrinter::Res(const tPrinterRes **res, int32 index)
{
	if ((index < 0) || (index >= fNbRes))		return B_ERROR;
	*res = fRes + index;
	return B_OK;
}

status_t MDefinePrinter::Paper(const tPrinterPaper **paper, int32 index)
{
	if ((index < 0) || (index >= fNbPaper))		return B_ERROR;
	*paper = fPaper + index;
	return B_OK;
}

status_t MDefinePrinter::ColorMode(const tColorMode **color, int32 index)
{
	if ((index < 0) || (index >= fNbColorMode))	return B_ERROR;
	*color = fColorMode + index;
	return B_OK;
}

status_t MDefinePrinter::WeaveMode(const tWeaveMode **weave, int32 index)
{
	if ((index < 0) || (index >= fNbWeaveMode))	return B_ERROR;
	*weave = fWeaveMode + index;
	return B_OK;
}

status_t MDefinePrinter::SpeedMode(const tSpeedMode **speed, int32 index)
{
	if ((index < 0) || (index >= fNbSpeedMode))	return B_ERROR;
	*speed = fSpeedMode + index;
	return B_OK;
}

status_t MDefinePrinter::DotSize(const tMicroDot **dot, int32 index)
{
	if ((index < 0) || (index >= fNbMicroDot))	return B_ERROR;
	*dot = fMicroDot + index;
	return B_OK;
}

status_t MDefinePrinter::FindRes2Paper(const tRes2Paper **res, int32 index)
{
	if (index == M_DEFAULT) {
		*res = fRes2Paper + fRes2Paper_default;
		return B_OK;
	}
	if ((index < 0) || (index >= fNbAllowed))	return B_ERROR;
	*res = fRes2Paper + index;
	return B_OK;
}

MColorProcess *MDefinePrinter::instantiate_MColorProcess(tPrinterDef *printer_def)
{
	return NULL;
}


// ------------------------------------------------------------------------------
// #pragma mark -

bool MDefinePrinter::IsEpsonModule() {
	return (fAttributes & EP_IS_EPSON_MODULE);
}
short MDefinePrinter::sSEInit(tagSE_INIT_PARAM *param) {
	return B_ERROR;
}
short MDefinePrinter::sSEOut(tagSE_OUT_PARAM *param) {
	return B_ERROR;
}
short MDefinePrinter::sSEEnd(tagSE_END_PARAM *param) {
	return B_ERROR;
}

// ------------------------------------------------------------------------------
// #pragma mark -


status_t MDefinePrinter::ioctl(uint32 cmd, void *arg1)
{
	switch (cmd)
	{
		case IOCTL_GET_EPSON_REMOTE_LEVEL:	*(uint32 *)arg1 = fPrinterInfo.gEpsonRemoteLevel;		return B_OK;
		case IOCTL_GET_FLOW_CONTROL:		*(uint32 *)arg1 = fPrinterInfo.gSerialFlowControl;		return B_OK;
		case IOCTL_GET_SERIAL_MODE:			*(uint32 *)arg1 = fPrinterInfo.gSerialSyncMode;			return B_OK;
		case IOCTL_GET_EXPANDED_SUPPORT:	*(bool *)arg1 = (fAttributes & EP_SUPPORTS_EXPANDED);	return B_OK;
		case IOCTL_PARSE_STATUS:
		{
			const bool six = (fAttributes & EP_IS_PHOTO);
			BMessage *msg = (BMessage *)arg1;
			const char *status;
			if (msg->FindString("epst:status", &status) != B_OK)
				return B_ERROR;
			int32 st,er,c,m,y,k,lc,lm;					
			st = er = c = m = y = k = lc = lm = -1;
			const char *p = strstr(status, "ST:");
			if (p) {
				sscanf(p, "ST:%02lX", &st);
				msg->AddInt32("epst:stat", st);
				if (st == 0) {
					p = strstr(status, "ER:");
					if (p) {
						sscanf(p, "ER:%02lX", &er);
						msg->AddInt32("epst:error", er);
					}
				}
			}

			p = strstr(status, "IQ:");
			if (p) {
				if (six) {
					sscanf(p, "IQ:%02lX%02lX%02lX%02lX%02lX%02lX", &k, &c, &m, &y, &lc, &lm);
					msg->AddInt32("epst:light_cyan",	lc);
					msg->AddInt32("epst:light_magenta",	lm);
				} else {
					sscanf(p, "IQ:%02lX%02lX%02lX%02lX", &k, &c, &m, &y);
				}
				msg->AddInt32("epst:black",		k);
				msg->AddInt32("epst:cyan",		c);
				msg->AddInt32("epst:magenta",	m);
				msg->AddInt32("epst:yellow",	y);
			}

			p = strstr(status, "TC:");
			if (p) {
				if (!fOpStartTime)
					fOpStartTime = system_time();
				int32 t;
				sscanf(p, "TC:%04lu", &t);
				t -= (system_time() - fOpStartTime)/1000000;
				if (t<0) t=0;
				msg->AddInt32("epst:time_sec", t);
			} else {
				fOpStartTime = 0;
			}
		}
		return B_OK;
	}
	return B_ERROR;
}

