//  PasswordPanel.h
//
//	brad modified russ 5/21/98
// 
//	(c) 1997-98 Be, Inc. All rights reserved


#ifndef _PASSWORD_H
#define _PASSWORD_H

#include <Window.h>
#include <RadioButton.h>

class PasswordPanel : public BWindow 
{
	public:
		PasswordPanel();
			
		~PasswordPanel();
			
		virtual void MessageReceived(BMessage *msg);
		

	private:
		void CreatePasswordUI();
		int check_password(void);
		void	Dependencies();

		char m_Password[256];

		BButton*		m_okbut;
		BButton*		m_cancelbut;
		BRadioButton	*netradio;
		BRadioButton	*customradio;
		BBox			*custombox;
		BView*			m_background;
		
		void SetPassword(const char *pass);
	
	public:
		BTextControl*	m_pass1;
		BTextControl*	m_pass2;
};



#endif /* _PASSWORD_H */
