//******************************************************************************
//
//	File:			IWindow.h
//
//	Description:	Installer window header.
//
//	Written by:		Steve Horowitz
//
//	Copyright 1994, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#ifndef IWINDOW_H
#define IWINDOW_H


#include <OS.h>
#include <Window.h>

#include "ProgressBar.h"
#include "Engine.h"

class TEngine;
class InitializeControlBlock;

class TIWindow : public BWindow {

public:
					TIWindow(TEngine*);
virtual				~TIWindow();
virtual void		MessageReceived(BMessage*);
virtual void		Minimize(const bool);
virtual	bool		QuitRequested();

		void		SetMessage(const char*);
		void		SetButton(const char*);

		InitializeControlBlock *InitializeParams();
		off_t InstallFileSize();
		off_t InstallAttrSize();
		off_t InstallNum();
		void SetInstallSizes(off_t, off_t, off_t);
		void SetSizeBarVisible(bool);
		void SetSizeBarMaxValue();
		void SetBarberPoleVisible(bool);
		void Update(float);

private:
		TEngine*	fEngine;
		InitializeControlBlock initParams;	// structure used to synchronize
											// the init addon and the caller
		
		// Sizes (in bytes) and numbers of files of base install
		// These numbers are read off disk from a special file
		off_t m_install_file_size;
		off_t m_install_attr_size;
		off_t m_install_num;

		typedef BWindow inherited;
};

inline InitializeControlBlock *TIWindow::InitializeParams()
	{ return &initParams; }


inline void TIWindow::SetInstallSizes(off_t file_size, off_t attr_size, off_t num)
{
	m_install_file_size = file_size;
	m_install_attr_size = attr_size;
	m_install_num = num;
}


inline off_t TIWindow::InstallFileSize()
{
	return m_install_file_size;
}

inline off_t TIWindow::InstallAttrSize()
{
	return m_install_attr_size;
}

inline off_t TIWindow::InstallNum()
{
	return m_install_num;
}

#endif
