/*************************************************************************
/
/	VirtScreenModule.h
/
/	Modified by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#ifndef _VIRT_SCREEN_MODULE_H_
#define _VIRT_SCREEN_MODULE_H_

#include <SupportDefs.h>


//========================================================================

class VirtScreenModule
{
	public:
							VirtScreenModule	()
													{};
		virtual void		Reset				()
													{};
		virtual void		ModeRlx				(int32 flag)
													{};
		virtual void		OutScr				(unsigned char s)
													{};
		virtual int32		GetLigne			()
													{ return 0; };
		virtual int32		GetColonne			()
													{ return 0; };
		virtual void		SetConnectStat		(int32 status)
													{};
};
#endif
