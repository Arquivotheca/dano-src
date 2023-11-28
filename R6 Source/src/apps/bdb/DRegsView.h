/*	$Id: DRegsView.h,v 1.2 1998/11/17 12:16:38 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/08/98 15:10:59
*/

#ifndef DREGSVIEW_H
#define DREGSVIEW_H

enum RegKind {
	rkNone,
	rkLong,
	rkShort,
	rkFloat
};

struct RegInfo {
	char name[16];			// name for this register
	int offset;				// offset in cpu_state record
	int size;					// size
	int bits;					// nr of significant bits
	BTextControl *ctrl;	// the textcontrol in the window
};

class DRegsView : public BView {
public:
			DRegsView(BRect frame, const char *name, int resID);
			~DRegsView();
			
virtual	void GetPreferredSize(float *x, float *y);
virtual	void Draw(BRect update);
virtual	void AttachedToWindow();
virtual	void SetTarget(BHandler *target);

virtual	void MessageReceived(BMessage *msg);

private:
			void ParseResource(BPositionIO& data);
			void NewCpuState();
			void RegModified(BMessage *msg);

			RegInfo *fRegInfo;
			int fRegInfCnt;
			char *fCpuState;
			int fCpuStateLen;
			int fResID;
			BHandler *fTarget;
};

extern BResources* gAppResFile;

#endif
