//========================================================================
//	BResPlugInView.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Jon Watte

#ifndef _BRESPLUGINVIEW_H
#define _BRESPLUGINVIEW_H

#include "MPlugInPrefsView.h"
#include "MPlugInBuilder.h"

struct ResPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32		version;
};

#if defined(__POWERPC__) || defined(__ARMEB__)	/* FIXME: This should probably use <endian.h> for the right define */
const ulong kResMessageType = 'RsDt';
#elif defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
const ulong kResMessageType = 'tDsR';
#endif

const char kResMessageName[] = "ResData";
const ulong kCurrentVersion = 0x00000001;



class BResPlugInView : public MPlugInPrefsView
{
public:
								BResPlugInView(
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

	ResPrefs					fOldSettings;
	ResPrefs					fNewSettings;

	void						ExtractInfo();

	void						SetGrey(
									BView* 		inView);
};

#endif
