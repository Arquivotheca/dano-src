#ifndef _UPDATEDIALOG_H_
#define _UPDATEDIALOG_H_


#include "StatusDialog.h"

// ReplaceDialog.h

class ManagerListView;
class UpdateThread;
class BInvoker;

class UpdateDialog : public StatusDialog
{
public:
	UpdateDialog(ManagerListView	*lv);
					
	//virtual			~UpdateDialog();
	
	virtual void	MessageReceived(BMessage *msg);
	virtual bool	QuitRequested();
	//status_t		Go(BInvoker *invoker);
private:
	BInvoker			*invoker;
	ManagerListView		*lv;
	
	UpdateThread		*updtThread;
	BMessage			data;
	int32				updateCount;
};


/***
enum {
	M_RADIO_SET	=	'RaSe'
};
***/

/***
class UpdateView : public BView
{
public:
	UpdateView(	BRect	frame);

	virtual void		AttachedToWindow();
	virtual void		Draw(BRect up);
private:
};
***/

/***
enum {
	UPDATE_checkOnly,
	UPDATE_checkDl,
	UPDATE_checkForceDl
};
***/

#endif
