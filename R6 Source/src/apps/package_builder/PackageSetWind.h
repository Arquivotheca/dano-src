// PackageSetWind.h

#include "ChildWindow.h"

enum {
	S_PACK_NAME =		'SPNa',
	S_PACK_VERS =		'SPVe',
	S_PACK_DEV	=		'SPDe',
	S_PACK_DATE =		'SPDa',
	S_PACK_DESC =		'SPDs',
	S_IS_UPDATE =		'SIsU',
	S_DEPOT_SERIAL =	'SDSn',
	S_SOFT_TYPE =		'SSTp',
	S_PREFIX_ID =		'SPfx',
	S_VERSION_ID =		'SVsI',
	S_DO_REG =			'SDoR',
	S_DO_UPDATE =		'SDoU'
};

class AttribData;

class PackageSetWind : public ChildWindow
{
public:
	PackageSetWind(	const char *title,
					BWindow *parW,
					AttribData *_attrib,
					bool *_dirtyFlag);
					
	~PackageSetWind();

	virtual bool		QuitRequested();
	virtual void		WindowActivated(bool state);
	virtual void		MessageReceived(BMessage *msg);

	AttribData			*attrib;
	bool				*dirtyFlag;
	
		char			*lastDate;
	
		void			SetName();
		void			SetVers();
		void			SetDev();
		void			SetDate();
		void			SetDesc();
		void			SetSerial();
};


class PackageSetView : public BView
{
public:
	PackageSetView( BRect r );
	
	virtual void		AttachedToWindow();
	virtual void		Draw(BRect up);
};
