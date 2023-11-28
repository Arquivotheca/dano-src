#ifndef	NETWORKINGCODE_INCLUDED
#define	NETWORKINGCODE_INCLUDED 1

#include "Settings.h"
#include <Looper.h>

class TNetDevScanList;

class NetworkingCore : public BLooper
{
public:
					NetworkingCore();
					~NetworkingCore();
	bool 			TryToSave(bool ask);
	Settings		settings;
	TNetDevScanList	*fNetDevScanList;

private:	
	void			MessageReceived(BMessage *msg);
	void 			Restart();
};		

#endif	// NETWORKINGCODE_INCLUDED
