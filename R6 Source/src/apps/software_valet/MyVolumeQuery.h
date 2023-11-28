#ifndef _MYVOLUMEQUERY_H_
#define _MYVOLUMEQUERY_H_


#include "MyDebug.h"

enum {
	M_VOL_INFO	= 'VInf'
};

#if 0
class MyVolumeQuery : public BQuery
{
public:
		MyVolumeQuery(BHandler	*target)
			:	BQuery(TRUE),
				fTarget(target)
		{};
			
virtual void MessageReceived(BMessage *msg)
			{
				PRINT(("VOLUME query\n"));
				if (msg->what == B_RECORD_MODIFIED) {
					fTarget->Looper()->PostMessage(M_VOL_INFO,fTarget);
				}
			};
private:		
		BHandler		*fTarget;
};
#endif


#endif
