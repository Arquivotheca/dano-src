#ifndef SPACE_CONVERSION_H
#define SPACE_CONVERSION_H

#include <BeBuild.h>
#include <ByteOrder.h>
#include <SupportDefs.h>

class MCS_Conversion
{
public:
			MCS_Conversion();
	virtual	~MCS_Conversion();
	
	virtual bool SpaceConversion(uint32 rgb, int16 *cmyk_i) = 0;

protected:
	bool IsMMX(void) { return fIsMMX; }

	struct interp_correct_t
	{
		uint8 level;
		uint8 delta;
	};
	
	interp_correct_t *fInterpolationTable;

private:
	bool fIsMMX;
};


#endif
