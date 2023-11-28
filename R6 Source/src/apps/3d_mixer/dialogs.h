#ifndef DIALOGS_H
#define DIALOGS_H

#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef	_TEXT_CONTROL_H
#include <TextControl.h>
#endif

 
class	AskBeat : public BWindow
{
public:
							AskBeat(float duration);
							~AskBeat();
virtual		void			MessageReceived(BMessage *msg);

private:
			BView			*master;
			BTextControl	*cur_beat;
			void			new_value();

};

//--------------------------------------------------------------
	
class	AskFilter : public BWindow
{
public:
							AskFilter(char *title);
							~AskFilter();
virtual		void			MessageReceived(BMessage *msg);

private:
			BView			*master;
			void			new_value();

};

//--------------------------------------------------------------
	

class	AskStrength : public BWindow
{
public:
							AskStrength(char *title);
							~AskStrength();
virtual		void			MessageReceived(BMessage *msg);

private:
			BView			*master;
			void			new_value();

};

//--------------------------------------------------------------
	

class	AskVolume : public BWindow
{
public:
							AskVolume(char *title);
							~AskVolume();
virtual		void			MessageReceived(BMessage *msg);

private:
			BView			*master;
			void			new_value();

};

//--------------------------------------------------------------
	




#endif
