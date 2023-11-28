/*	$Id: DRegsWindow.h,v 1.4 1999/02/03 08:30:03 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/08/98 14:28:22
*/

#ifndef DREGSWINDOW_H
#define DREGSWINDOW_H

#include "DListBox.h"
#include "DThread.h"

#include <Window.h>

class DThread;
class DRegsWindow;

class DRegsItem : public DListItem
{
  public:
	DRegsItem(DRegsWindow *window, const char *name, int offset, int length, short type)
		: DListItem(name), fWindow(window), fOffset(offset), fLength(length), fType(type) {}
	
	virtual const char* GetColumnText(int column);
	virtual void SetColumnText(int column, const char *newText);
	virtual void DrawItem(BView *owner, BRect bounds, bool complete, int column);

	// registers can be up to 16 bytes
	void GetCPUValue(void* outValue);
	void UpdateValue();

  private:
  	DRegsWindow *fWindow;
  	int fOffset, fLength;
  	short fType;
  	char fValue[64];
  	bool fChanged;
};

class DRegsWindow : public BWindow
{
  public:
	DRegsWindow(BRect frame, const char *name, DThread& thread, int resID);

	virtual bool QuitRequested();
	virtual void SetTarget(BHandler *target);
	virtual void MessageReceived(BMessage *msg);
	
	const DCpuState& RegsData() const	{ return *fCPU; }
	
  protected:
  
  	void DumpMemory();
  	void AddWatchpoint();
	ptr_t GetSelectedRegisterValue();

	DThread& fThread;  	
	DListBox *fList;
	DCpuState* fCPU;
	BHandler *fTarget;
};

#endif
