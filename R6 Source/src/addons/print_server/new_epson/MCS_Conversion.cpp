#include <OS.h>
#include <math.h>

#include "MCS_Conversion.h"

MCS_Conversion::MCS_Conversion()
	:	fInterpolationTable(NULL), fIsMMX(false)
{
#ifdef __INTEL__
	cpuid_info info;
	if (get_cpuid(&info, 1, 0) == B_OK)	// Request for feature flag (1)
		fIsMMX = (info.eax_1.features & 0x800000);
#endif
	fInterpolationTable = new interp_correct_t[256];
#if 0
	const float gamma = 1.0;
	const float sl = 1.0/224.0;
	const float si = 1.0/7.0;
#endif
	for (int j=0 ; j<256 ; j++)
	{
		const int l = (int)((j*223)/255);
#if 0
		const int i = (l*7)/224;
		const float x0 = i*si;
		const float x1 = (i+1)*si;
		const float y0 = pow(x0, gamma);
		const float y1 = pow(x1, gamma);
		const float x = l*sl;
		const float y = pow(x, gamma);
		const float z = x0 + (y - y0) * ( si / (y1 - y0) );
		const int ix = (int)(z*223);
		fInterpolationTable[j].level = i;
		fInterpolationTable[j].delta = ix-(int)(x0*223);
#endif
		fInterpolationTable[j].level = l >> 5;
		fInterpolationTable[j].delta = l & 0x1F;
	}
}

MCS_Conversion::~MCS_Conversion(void)
{
	delete [] fInterpolationTable;
}

