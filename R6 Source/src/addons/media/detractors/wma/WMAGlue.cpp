
#include <stdio.h>
#include "WMA_Dec_Emb_x86.h"
#include "WMADetractor.h"

//#define DEBUG printf
#define DEBUG if (0) printf
#define SEEK(x) //printf x
#define SEQ(x) //printf x

extern "C" tWMA_U32 WMAFileCBGetData (
    tHWMAFileState hstate,
    tWMA_U32 offset,
    tWMA_U32 num_bytes,
    unsigned char **ppData)
{
//	DEBUG("WMAFileCBGetData(%08x,%d,%d,%08x)\n",hstate,offset,num_bytes,ppData);

	return currentdetractor->WMAFileCBGetData(hstate,offset,num_bytes,ppData);
}

tWMA_U32 WMADetractor::WMAFileCBGetData(
		    tHWMAFileState /*hstate*/,
		    tWMA_U32 offset,
		    tWMA_U32 num_bytes,
		    unsigned char **ppData)
{
//	DEBUG("WMADetractor(%08x)::WMAFileCBGetData(%08x,%d,%d,%08x)\n",this,hstate,offset,num_bytes,ppData);

    tWMA_U32 ret;

	tWMA_U32 nWanted = num_bytes <= (tWMA_U32) WMA_MAX_DATA_REQUESTED ? num_bytes : (tWMA_U32) WMA_MAX_DATA_REQUESTED;
    if(num_bytes != nWanted)
    {
        fprintf(stderr, "** WMAFileCBGetData: Requested too much (%lu).\n", num_bytes);
    }

	ret = fSource->ReadAt(offset,g_pBuffer, num_bytes);

    g_cbBuffer = ret;

    *ppData = g_pBuffer;

//	fprintf(stderr, "++ WMAFileCBGetData: %lu bytes from %lu.\n",
//		ret, offset);

    return ret;
}


tWMA_U32 WMAFileCBGetLicenseData (
    tHWMAFileState *state,
    tWMA_U32 offset,
    tWMA_U32 num_bytes,
    unsigned char **ppData)
{
	DEBUG("WMAFileCBGetLicenseData(%08x,%u,%u,%08x)\n",int(state),int(offset),int(num_bytes),int(ppData));
	return currentdetractor->WMAFileCBGetLicenseData(state,offset,num_bytes,ppData);
}

tWMA_U32 WMADetractor::WMAFileCBGetLicenseData (
    tHWMAFileState *state,
    tWMA_U32 offset,
    tWMA_U32 num_bytes,
    unsigned char **ppData)
{
	DEBUG("WMADetractor(%08x)::WMAFileCBGetLicenseData(%08x,%d,%d,%08x)\n",int(this),int(state),int(offset),int(num_bytes),int(ppData));
	tWMA_U32        ret;
	unsigned char  *g_plicData;
	tWMA_U32        g_LicenseLength;
	tWMA_U32        nWanted;

	g_plicData = WMAGetLicenseStore((tWMAFileHdrState*)g_state, &g_LicenseLength);

    nWanted = num_bytes <= (tWMA_U32) WMA_MAX_DATA_REQUESTED ? num_bytes : (tWMA_U32) WMA_MAX_DATA_REQUESTED;
    if(num_bytes != nWanted)
    {
        fprintf(stderr, "** WMAFileCBGetLicenseData: Requested too much (%lu).\n",
                num_bytes);
    }


    if (g_LicenseLength) {
        g_cbBufLic = ((offset +nWanted) > g_LicenseLength) ? (g_LicenseLength - offset): nWanted;
        ret = g_cbBufLic;
        *ppData = g_plicData + offset;
    }
    else {
        fseek (g_fpLic, offset, SEEK_SET);
        ret = fread (g_pBufLic, 1, (size_t)nWanted, g_fpLic);

        g_cbBufLic = ret;

        *ppData = g_pBufLic;

    }

    fprintf(stderr, "++ WMAFileCBGetLicenseData: %lu bytes from %lu.\n", ret, offset);

    return ret;
}


void WMADebugMessage(const char* pszFmt, ... )
{
    va_list vargs;
    va_start(vargs, pszFmt);
    vfprintf(stderr, pszFmt, vargs );
    va_end(vargs);
}
