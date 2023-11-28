#ifndef _REGISTERVIEW_H_
#define _REGISTERVIEW_H_

// RegisterView.h

#include <Rect.h>
#include <View.h>
#include <Message.h>
#include <Messenger.h>


class PackageItem;
class RegisterThread;

class RegisterView : public BView
{
public:
	RegisterView(BRect b, BMessage &pkgs, BMessenger _updt,bool showSerial);
	
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *);
	
			bool	ValidateRegInfo(BMessage *msg);
			void	MakeEditable(bool state);
			
	BMessage		&curPkgs;		// packages to be registered
	BMessage 		curReg;		// current registration info
	
	enum {
		M_REGLATER	= 'RLat',
		M_REGNOW	= 'RNow',
		M_REGOK		= 'ReOK',
		M_SAVEREG	= 'RSRe'
	};
	long			regWhen;
	long			registerCount;
	
	bool			needSerialID;
	bool			saveReg;
	BMessenger		updt;
	RegisterThread	*regThread;
};

#endif
