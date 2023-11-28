//========================================================================
//	MShellPlugInView.h
//	Copyright 1996 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MSHELLPLUGINVIEW_H
#define _MSHELLPLUGINVIEW_H

#include "MPlugInPrefsView.h"

// The shellprefs struct is stored in the project in big endian format
// We swap to host endian format when we read the struct from
// the message or before writing the struct to the message.
struct ShellPrefs
{
	void		SwapHostToBig();
	void		SwapBigToHost();

	int32		version;
	char		options[256];
};

const char kShellMessageName[] = "ShellData";
const ulong kCurrentVersion = 0x00000001;
#if B_HOST_IS_BENDIAN
const ulong kShellMessageType = 'ShDt';
#else
const ulong kShellMessageType = 'tDhS';
#endif

class MTextView;


class MShellPlugInView : public MPlugInPrefsView
{
public:
								MShellPlugInView(
									BRect			inFrame,
									const char*		inName,
									ulong			inResizeMask,
									ulong			inFlags);

	virtual void				AttachedToWindow();

	virtual void				MessageReceived(
									BMessage*	inMessage);

	virtual const char *		Title();
	virtual TargetT				Targets();

	virtual void				GetPointers(
									void*&	outOld,
									void*&	outNew,
									long&	outLength);

	virtual void				DoSave();
	virtual void				DoRevert();
	virtual void				DoFactorySettings();
	virtual	bool				FilterKeyDown(
									ulong aKey);	
	virtual	bool				ProjectRequiresUpdate(
									UpdateType inType);

	virtual void				GetData(
									BMessage&	inOutMessage);
	virtual void				SetData(
									BMessage&	inMessage);

	virtual void				ValueChanged();
	virtual void				UpdateValues();

private:

	MTextView*					fTextBox;
	ShellPrefs					fOldSettings;
	ShellPrefs					fNewSettings;

	void						ExtractInfo();

	void						SetGrey(
									BView* 		inView);
};

#endif
