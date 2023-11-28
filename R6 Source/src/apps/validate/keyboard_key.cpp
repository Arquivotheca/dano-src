#include "Test.h"

#include <Window.h>
#include <TextView.h>
#include <OS.h>
#include <stdio.h>
#include <Alert.h>
#include "ValidApp.h"
#include "settings.h"
#include <Button.h>

Test * make_keyboard_key();

static char*
get_key_name(char* key_name, char m_theKey)
{
	switch (m_theKey) {
		case B_BACKSPACE:
			strcpy(key_name, "Backspace key");
			break;
		case B_ENTER:
			strcpy(key_name, "Enter key");
			break;
		case B_SPACE:
			strcpy(key_name, "Space key");
			break;
		case B_TAB:
			strcpy(key_name, "Tab key");
			break;
		case B_ESCAPE:
			strcpy(key_name, "Escape key");
			break;
		case B_LEFT_ARROW:
			strcpy(key_name, "Left Arrow key");
			break;
		case B_RIGHT_ARROW:
			strcpy(key_name, "Right Arrow key");
			break;
		case B_UP_ARROW:
			strcpy(key_name, "Up Arrow key");
			break;
		case B_DOWN_ARROW:
			strcpy(key_name, "Down Arrow key");
			break;
		case B_DELETE:
			strcpy(key_name, "Delete key");
			break;
		case B_HOME:
			strcpy(key_name, "Home key");
			break;
		case B_END:
			strcpy(key_name, "End key");
			break;
		case B_PAGE_UP:
			strcpy(key_name, "Page Up key");
			break;
		case B_PAGE_DOWN:
			strcpy(key_name, "Page Down key");
			break;
		default:
			key_name[0] = m_theKey;
			key_name[1] = 0;
			break;
	}

	return key_name;
}

class KeyboardKeyView : public BTextView {
public:
						KeyboardKeyView(BRect area, char theKey);

	virtual	void		KeyDown(const char *bytes, int32 numBytes);

private:
	int					m_failCount;
	char				m_theKey;
		
};

KeyboardKeyView::KeyboardKeyView(BRect area, char theKey)
				: BTextView(area, "usertext", area.InsetByCopy(10,10), B_FOLLOW_ALL)
{
	m_theKey = theKey;
	m_failCount = 0;
}

void
KeyboardKeyView::KeyDown(const char *bytes, int32 numBytes)
{
	BTextView::KeyDown(bytes, numBytes);

	if (*bytes != m_theKey) {
		char msg[100];
		char saw_keyname[32];
		char looking_keyname[32];
		sprintf(msg, "Looking for '%s', but saw '%s'.", get_key_name(looking_keyname, m_theKey), get_key_name(saw_keyname, *bytes));
		m_failCount++;
		BAlert* alert = new BAlert("", msg, m_failCount < 3 ? "Again" : "Fail");
		SetLargeAlertFont(alert);
		alert->Go();
		if (m_failCount >= 3) {
			fail(msg);
			Window()->PostMessage('miss');
		}
	}
	else {
		Window()->PostMessage('good');
	}
}

class KeyboardKeyWindow : public TestWindow {
public:
	KeyboardKeyWindow() : TestWindow(BRect(100,100,400,300), "Keyboard input", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE) {
		
		// get the key that needs to be tested (or randomly generate one)
		char setting_buffer[256];
		char theKey;
		setting_buffer[0] = 0;
		get_setting("keyboard.keys", setting_buffer, sizeof(setting_buffer));
		if (setting_buffer[0] != 0) {
			if (strlen(setting_buffer) == 1) 
			{
				// single character - just set it
				theKey = setting_buffer[0];
			}
			else
			{
				// must be ASCII equivalent, convert it (if problem, default to ENTER)
				theKey = (char) get_setting_value("keyboard.keys", 10);
			}
		}
		else {
			theKey = (system_time()%27)+'a';
		}
		BTextView * tv = new BTextView(BRect(0,0,300,80), "text", BRect(10,10,290,70), B_FOLLOW_ALL);
		tv->SetFontSize(15);
		AddChild(tv);
		char msg[100];
		char key_name[256];
		sprintf(msg, "Press the '%s' on the keyboard.", get_key_name(key_name, theKey));
		tv->SetText(msg);
		
		BRect inputBox(0, 60, 300, 100);
		KeyboardKeyView* kv = new KeyboardKeyView(inputBox, theKey);
		AddChild(kv);
		kv->MakeFocus(true);
		BButton * fail = new BButton(BRect(10,165,90,195), "fail", "Fail", new BMessage('fail'));
		AddChild(fail);
	}

	void MessageReceived(BMessage * msg) {
		switch (msg->what) {
		case 'fail':
			fail("Keyboard input cancelled by operator\n");
			this->TestDone(false);
			break;
		case 'miss':
			this->TestDone(false);
			break;
		case 'good':
			this->TestDone(true);
			break;
		default:
			TestWindow::MessageReceived(msg);
			break;
		}
	}
};

Test* make_keyboard_key()
{
	KeyboardKeyWindow * kkw = new KeyboardKeyWindow;
	return kkw->GetTest();
}

