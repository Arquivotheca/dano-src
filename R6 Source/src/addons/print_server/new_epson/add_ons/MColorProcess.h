

#ifndef MCOLOR_PROCESS_H
#define MCOLOR_PROCESS_H

#include "MDefinePrinter.h"


// -----------------------------------------------------
// class name: MColorProcess
// purpose: Color processing for this printer
// -----------------------------------------------------

class MColorProcess
{
public:
			MColorProcess	(tPrinterDef *printer_def);
virtual 	~MColorProcess	(void);

			const void *InitSpaceConversion(void) const
			{
				return fColorTable;
			}

private:
	const void		*fColorTable;
};

#endif

