#ifndef _CHINESE_H_
#define _CHINESE_H_

#include <Button.h>
#include <ListItem.h>
#include <ListView.h>
#include <Looper.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <String.h>
#include <TextView.h>
#include <Window.h>
#include <add-ons/input_server/InputServerMethod.h>

class ChineseLooper;
class ChineseMatchWindow;
//class TInputWindow;
//class TListView;
//class TInputView;
class TLabelWindow;

class ChineseInputMethod : public BInputServerMethod
{
public:
							ChineseInputMethod(const char *name, const uchar *icon);
	virtual 				~ChineseInputMethod();
	
	virtual filter_result	Filter(BMessage *message, BList *outList);
	virtual status_t		MethodActivated(bool active);

private:

	ChineseLooper*	fLooper;
};

class ChineseLooper : public BLooper
{
public:
							ChineseLooper(ChineseInputMethod *method);
	virtual					~ChineseLooper();

	virtual void			MessageReceived(BMessage *message);
			void			EnqueueMessage(BMessage *message);


private:

	typedef enum {
		EMPTY				= 0,
		SHOWING_TEXT		= 1,
		//SHOWING_CONVERTED	= 2,
		SHOWING_CHOICES		= 3
	} input_state;

			void			Trim(char *str);
			int				InitSub(); 
			int				OpenDict(const char* dictFile);
			int				CloseDict();

			int				Append(int8 theKey, int32 modifiers);
			void			SelectMatch(int8 matchIndex);
			void			DoBackspace();
			bool			DoConversion();
			void			RevertConversion();
			BMessage*		GenerateInputMethodEvent(const char *string, bool selected, bool confirmed);
			const char*		CurrentText();
			
			int				IsInputKey(int8 keycode, int8 *keyidx);
			//bool				IsArrowKey(int8 keycode, int32 *select1);
			//int				IsSelectKey(int8 keycode);
			//int				IsScrollKey(int8 keycode);
			bool			IsEndKey(int8 keycode);
			int				MatchDict(const char *dictEng, char **dictCh);
			int				GetMatch(int8 matchIndex, char *dest);
			
			//filter_result	InputFilter(BMessage *message);
			//void			Set_Match_List();
			//void			Get_Select_HZ();
			//void 			Get_Usr_Input(int8 keycode, int8 keyidx);
			//void			Del_Usr_Input();
			//void			Hide_All_Window();
			
			void			HandleMethodActivated(bool active);
			void			HandleKeyDown(BMessage *message);
			void			StartInput();
			void			StopInput(bool onlyStop = false, bool confirm = false);
			void			StartMatchWindow();
			void			StopMatchWindow();
			void			SetState(input_state state);
			void			ResetText();
			void			Reset();
				
	BMessenger				fSelf;
	BString					fEntered;		// the characters the user entered
	BString					fEnteredHanzi;	// fEntered converted to Hanzi
	BString					fConverted;		// a Hanzi character conversion of fEnteredHanzi
	ChineseInputMethod*		fOwner;
	ChineseMatchWindow*		fMatchWin;
	TLabelWindow*			fLabelWin;
	input_state				fState;
	int32					fMatchIndex;
	int32					fCurrentPage;
	//BMenu*					fMenu;
};


class TLabelWindow : public BWindow{
public:
					TLabelWindow(BRect frame, const char *title, ChineseLooper* owner);

	virtual void	MessageReceived(BMessage *message);

private:

	ChineseLooper*	fOwner;
	BMenu *menu;
	BMenuBar *menuBar;
};


class ChineseMatchWindow : public BWindow{
public:
							ChineseMatchWindow(BRect frame, ChineseLooper* owner);
		virtual				~ChineseMatchWindow();
		
				bool		Populate(int32 page = 0);
				
protected:

		virtual	void		MessageReceived(BMessage *message);
						
private:
	ChineseLooper*			fOwner;
	BListView*				fListView;
	int32					fCurrentPage;
};

class ChineseMatchItem : public BListItem {
public:
							ChineseMatchItem(int32 index, const char *hanzi);
	virtual					~ChineseMatchItem();

	virtual void		DrawItem(BView *owner, BRect bounds, bool complete = false);
	virtual void		Update(BView *owner, const BFont *font);

private:

	BString				fHanzi;
	BString				fIndex;
	float				fIndexWidth;
	float				fTextBaseline;
};
	
#endif // _CHINESE_H_
