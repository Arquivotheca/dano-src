//  FileAndPrintPanel.h
//
//	russ 1/15/99
// 
//	(c) 1997-99 Be, Inc. All rights reserved


#ifndef _FILEANDPRINT_H
#define _FILEANDPRINT_H

#include <Window.h>

#include "Resource.h"

class FileAndPrintPanel : public BWindow 
{
	public:
		FileAndPrintPanel();			
		~FileAndPrintPanel();
		bool QuitRequested();
			
		virtual void MessageReceived(BMessage *msg);
		
		
	private:
		void CreateFileAndPrintUI();
		int check_password(void);
		
		BButton*		m_okbut;
		BButton*		m_cancelbut;
		BView*			m_background;
		BTextControl*	m_workgroup;	
		BTextControl*	m_user;	
		BCheckBox*		m_cifsenable;
		BCheckBox*	    m_hideShares;
		bool			m_hideSharesFlag;
		
		void Update(int16 direction);
		
		
		char m_WorkgroupName[kMaxStrLen];
		char m_FPUserName[kMaxStrLen];
		//  m_Password is in the encrypted form that
		// can be dropped into the network settings file
		char m_Password[kMaxStrLen];
		char m_ClearTextPass[kMaxStrLen];
		
		void SetPassword(const char *pass);
		void GetNetworkSettings();
		void SaveNetworkSettings();
		int GetNextFieldStr(char* instring, char* outstring);
		void RestartNetworking();
	
	public:
		BTextControl*	m_pass1;
		BTextControl*	m_pass2;
};



#endif /* _FILEANDPRINT_H */

