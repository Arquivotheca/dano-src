#include "TeamMonitorWindow.h"
#include "kb_mouse_driver.h"
#include "priv_syscalls.h"

#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Debug.h>
#include <image.h>
#include <ListView.h>
#include <ListItem.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <SymLink.h>
#include <TextView.h>
#include <stdio.h>
#include <unistd.h>

const float		kDisabledLoc	= -10000.0;
const float 	kWindowWidth 	= 350.0;
const float 	kWindowHeight 	= 300.0;
const uint32	msg_Kill		= 'Kill';
const uint32	msg_Reboot		= 'Boot';
const uint32	msg_CheckTeams	= 'CTms';
const uint32	msg_Ping		= 'Ping';
const uint32	msg_Disable		= 'Dsab';
const uint32	msg_Raise_pri	= 'Rpri';
const uint32 	msg_Launch_term = 'Trmn';
const uint32 	msg_Launch_trak = 'Trkr';

const char*		kInitialText	= "Select an application from the list above and click the \"Kill\" "
								  "button in order to close it.\n\n"
								  "Hold CONTROL+ALT+DELETE for 4 seconds to reboot.";
const char*		kKillText		= "Select a team from the list above and click the \"Kill\" "
								  "button in order to kill it.\n";
const char*		kSystemText		= "(This team is a component of the BeOS)";
const char*		kCrazyText		= "(This team may be misbehaving)";

#define kTerminalSig 	"application/x-vnd.Be-SHEL"
#define kTrackerSig 	"application/x-vnd.Be-TRAK"
#define kDeskbarSig 	"application/x-vnd.Be-TSKB"

class _TeamInfoView_ : public BView {
public:
						_TeamInfoView_(BRect frame);

	virtual void		AttachedToWindow();
	virtual void		Draw(BRect updateRect);
	virtual void		GetPreferredSize(float *width, float *height);

	void				SetKillButton(BButton *killButton);
	
	void				SetTeam(const team_info	*team, 
								const BBitmap	*bitmap,
								bool			systemServer = false,
								bool			crazy = false);

private:
	void				AddInitialText();

private:
	const BBitmap*		fBitmap;
	const char*			fArgs;
	BTextView*			fInitialText;
	BButton*			fKillButton;
	bool				fSystemServer;
	bool				fCrazy;
};


class _TeamListItem_ : public BStringItem {
public:
						_TeamListItem_(team_info *tInfo, const BPath* path,
									   bool systemServer, bool crazy);
	virtual				~_TeamListItem_();

	virtual void		DrawItem(BView *owner, BRect frame, bool complete = false);
	virtual void		Update(BView *owner, const BFont *font);

	team_id				Team() const;
	const team_info*	TeamInfo() const;
	const BBitmap*		Icon(icon_size size) const;
	bool				SystemServer() const;
	bool				Crazy() const;

private:
	team_info			fTeamInfo;
	rgb_color			fHighlightColor;
	BBitmap*			fMiniBitmap;
	BBitmap*			fLargeBitmap;
	float				fTextBaseline;
	bool				fSystemServer;
	bool				fCrazy;
};


class _TeamListView_ : public BListView {
public:
						_TeamListView_(BRect frame, _TeamInfoView_ *infoView);

	virtual void		MessageReceived(BMessage *message);
	virtual void		SelectionChanged();

	void				Enable();
	void				Disable();

	void				Kill();
	void				CheckTeams();

private:
	static int32		MessageSender(void *arg);

private:
	_TeamInfoView_*		fInfoView;
	thread_id			fMessageSender;
	bool				fSendMessages;
	
};


thread_id*	TeamMonitorWindow::sThreadID = NULL;
uint8*		TeamMonitorWindow::sThreadMonitor = NULL;
uint32		TeamMonitorWindow::sThreadMonitorSize = 0;
int32*		TeamMonitorWindow::sCrazyThreadsCount = NULL;
int32*		TeamMonitorWindow::sCrazyThreads = NULL;
bool		TeamMonitorWindow::sEnabled = false;


static void
get_path_for_team_info(
	team_info	*tInfo,
	BPath		*path)
{
	BString	string(tInfo->args);
	int32	stringMax = string.Length() - 1;

	if (string[stringMax] == ' ')
		string.Truncate(stringMax);

	for (int32 i = 1; i < tInfo->argc; i++) {
		int32 last = string.FindLast(' ');

		if (last < 0)
			break;
			
		string.Truncate(last);
	}

	path->SetTo(string.String());
	if (path->InitCheck() != B_OK) {
		// There was a failure getting the path.  Most often, this
		// is because the first argument is a relative path.  In this
		// case, take the path of the first image in the team.
		image_info image;
		int32 cookie=0;
		while (get_next_image_info(tInfo->team, &cookie, &image) == B_OK) {
			if (image.type == B_APP_IMAGE && path->SetTo(image.name) == B_OK)
				break;
		}
	}
}


_TeamInfoView_::_TeamInfoView_(
	BRect	frame)
		: BView(frame, B_EMPTY_STRING, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fBitmap = NULL;
	fArgs = NULL;
	fInitialText = NULL;
	fKillButton = NULL;

	AddInitialText();
}


void
_TeamInfoView_::AttachedToWindow()
{
	rgb_color viewColor = Parent()->ViewColor();
	
	SetViewColor(viewColor);
	SetDrawingMode(B_OP_OVER);

	if (fInitialText != NULL)
		fInitialText->SetViewColor(viewColor);
}


void
_TeamInfoView_::Draw(
	BRect	updateRect)
{
	(void)updateRect;
	
	BRect	bounds = Bounds();
	BPoint	leftTop = bounds.LeftTop();
	//leftTop.y = (bounds.Height() - 32.0) / 2.0;
	
	font_height fh;
	GetFontHeight(&fh);
	
	bool	addText = fCrazy || fSystemServer;
	
	float	textHeight = ceil(fh.ascent+fh.descent+fh.leading) * (addText ? 2 : 1);
	BPoint	drawLoc = BPoint(40.0, floor((32-textHeight)/2) + fh.ascent);	
	
	if (drawLoc.y < 0) {
		leftTop.y -= drawLoc.y;
		drawLoc.y = 0;
	}
	
	if (fBitmap != NULL)
		DrawBitmapAsync(fBitmap, leftTop);

	if (fArgs != NULL) 
		DrawString(fArgs, leftTop + drawLoc);

	if (addText) {
		drawLoc.y += ceil(fh.ascent + fh.descent + fh.leading);		
		DrawString((fCrazy) ? kCrazyText : kSystemText, leftTop + drawLoc);
	}
}


void
_TeamInfoView_::GetPreferredSize(
	float	*width,
	float	*height)
{
	BView::GetPreferredSize(width, height);

	*height = (fInitialText != NULL) ? ceil(fInitialText->TextHeight(0, LONG_MAX)) : 32.0;
}


void
_TeamInfoView_::SetKillButton(BButton *killButton)
{
	fKillButton = killButton;
}


void
_TeamInfoView_::SetTeam(
	const team_info	*team, 
	const BBitmap 	*bitmap,
	bool			systemServer,
	bool			crazy)
{
	fBitmap = bitmap;

	if (team == NULL) {
		fArgs = NULL;

		if (fInitialText == NULL)
			AddInitialText();

		if (fKillButton != NULL)
			fKillButton->SetEnabled(false);

		fSystemServer = false;
		fCrazy = false;
	}
	else {
		fArgs = team->args;
		
		if (fInitialText != NULL) {
			fInitialText->RemoveSelf();
			delete (fInitialText);
			fInitialText = NULL;
		}

		if (fKillButton != NULL)
			fKillButton->SetEnabled(true);

		fSystemServer = systemServer;
		fCrazy = crazy;
	}

	BWindow *window = Window();
	if (window != NULL) {
		Invalidate();
		window->UpdateIfNeeded();
	}
}


void
_TeamInfoView_::AddInitialText()
{
	if (fInitialText != NULL)
		return;

	BRect textFrame = Bounds();
	BRect textRect = textFrame;
	textRect.OffsetTo(B_ORIGIN);
		
	fInitialText = new BTextView(textFrame, B_EMPTY_STRING, textRect, B_FOLLOW_ALL_SIDES);
	fInitialText->MakeEditable(false);
	fInitialText->MakeSelectable(false);
	fInitialText->SetText(kInitialText);
	fInitialText->SetViewColor(ViewColor());
	
	AddChild(fInitialText);

	textRect.InsetBy(2.0, 2.0);
	fInitialText->SetTextRect(textRect);
}


_TeamListItem_::_TeamListItem_(
	team_info	*tInfo,
	const BPath	*path,
	bool		systemServer,
	bool		crazy)
		: BStringItem(B_EMPTY_STRING)
{
	const rgb_color	kBlack = {0, 0, 0, 255};
	const rgb_color	kRed = {255, 0, 0, 255};
	const rgb_color kBlue = {0, 0, 255, 255};

	memcpy(&fTeamInfo, tInfo, sizeof(fTeamInfo));
	fHighlightColor = kBlack;
	fMiniBitmap = new BBitmap(BRect(0.0, 0.0, 15.0, 15.0), B_CMAP8);
	fLargeBitmap = new BBitmap(BRect(0.0, 0.0, 31.0, 31.0), B_CMAP8);
	fTextBaseline = 0.0;
	fSystemServer = systemServer;
	fCrazy = crazy;

	if (fCrazy)
		fHighlightColor = kRed;
	else if (fSystemServer)
		fHighlightColor = kBlue;

	BPath teamPath(*path);
	if (teamPath.InitCheck() != B_OK)
		teamPath.SetTo("/<unknown path>");
	
	BNode teamNode(teamPath.Path());

	while (teamNode.IsSymLink()) {
		BSymLink	symlink(teamPath.Path());
		BPath		parentPath;
		teamPath.GetParent(&parentPath);

		symlink.MakeLinkedPath(parentPath.Path(), &teamPath);
		teamNode.SetTo(teamPath.Path());
	}

	BNodeInfo teamNodeInfo(&teamNode);
	if (teamNodeInfo.InitCheck() == B_OK) {
		teamNodeInfo.GetTrackerIcon(fMiniBitmap, B_MINI_ICON);
		teamNodeInfo.GetTrackerIcon(fLargeBitmap, B_LARGE_ICON);
	}
	
	SetText(teamPath.Leaf());
}


_TeamListItem_::~_TeamListItem_()
{
	delete (fMiniBitmap);
	delete (fLargeBitmap);
}


void
_TeamListItem_::DrawItem(
	BView	*owner, 
	BRect	frame, 
	bool	complete)
{
	(void)complete;
	
	drawing_mode	saveMode = owner->DrawingMode();
	rgb_color		saveColor = owner->LowColor();
	rgb_color		kWhite = {255, 255, 255, 255};

	owner->SetDrawingMode(B_OP_COPY);

	owner->SetLowColor((IsSelected()) ? ui_color(B_PANEL_BACKGROUND_COLOR) : kWhite);
	owner->FillRect(frame, B_SOLID_LOW);
	owner->SetLowColor(saveColor);

	owner->SetDrawingMode(B_OP_OVER);

	BPoint drawLoc = frame.LeftTop();
	drawLoc.x += 4.0;
	drawLoc.y += (frame.Height() - 16.0) / 2.0;
	owner->DrawBitmapAsync(fMiniBitmap, drawLoc);

	owner->MovePenTo(frame.left + 26.0, frame.top + fTextBaseline);
	owner->SetHighColor(fHighlightColor);
	owner->DrawString(Text());	

	owner->SetDrawingMode(saveMode);
}


void
_TeamListItem_::Update(
	BView		*owner, 
	const BFont	*font)
{
	BStringItem::Update(owner, font);

	font_height fontHeight;
	font->GetHeight(&fontHeight);

	float height = ceil(fontHeight.ascent + fontHeight.descent + fontHeight.leading) + 2.0;
	height = (height > 18.0) ? height : 18.0;

	SetHeight(height); 

	fTextBaseline = ceil(fontHeight.ascent + fontHeight.descent) + 1.0;
}


team_id
_TeamListItem_::Team() const
{
	return (fTeamInfo.team);
}


const team_info*
_TeamListItem_::TeamInfo() const
{
	return (&fTeamInfo);
}


const BBitmap*
_TeamListItem_::Icon(
	icon_size	size) const
{
	switch (size) {
		case B_MINI_ICON:
			return (fMiniBitmap);

		default:
		case B_LARGE_ICON:
			return (fLargeBitmap);
	}

	return (NULL);
}


bool
_TeamListItem_::SystemServer() const
{
	return (fSystemServer);
}


bool
_TeamListItem_::Crazy() const
{
	return (fCrazy);
}


_TeamListView_::_TeamListView_(
	BRect			frame,
	_TeamInfoView_	*infoView)
		: BListView(frame, B_EMPTY_STRING)
{
	fInfoView = infoView;
	fMessageSender = B_ERROR;
	fSendMessages = false;
}


void
_TeamListView_::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case msg_Kill:
			Kill();
			Window()->PostMessage(msg_Disable);
			break;

		case msg_CheckTeams:
			CheckTeams();
			break;

		default:
			BListView::MessageReceived(message);
			break;
	}
}


void
_TeamListView_::SelectionChanged()
{
	_TeamListItem_ *item = (_TeamListItem_ *)ItemAt(CurrentSelection());

	if (item == NULL)
		fInfoView->SetTeam(NULL, NULL);
	else
		fInfoView->SetTeam(item->TeamInfo(), item->Icon(B_LARGE_ICON), item->SystemServer(), item->Crazy());
}


void
_TeamListView_::Enable()
{
	CheckTeams();

	// don't use BMessageRunner as it depends on the registrar
	fSendMessages = true;
	fMessageSender = spawn_thread(MessageSender, "MessageSender", B_NORMAL_PRIORITY, this);
	resume_thread(fMessageSender);
}


void
_TeamListView_::Disable()
{
	int32 result = 0;

	fSendMessages = false;
	wait_for_thread(fMessageSender, &result);
	fMessageSender = B_ERROR;	

	_TeamListItem_ *item = NULL;
	while ((item = (_TeamListItem_* )RemoveItem((int32)0)) != NULL)
		delete (item);
}


void
_TeamListView_::Kill()
{
	_TeamListItem_ *item = (_TeamListItem_ *)ItemAt(CurrentSelection());

	if (item != NULL)
		kill_team(item->Team());
}


void
_TeamListView_::CheckTeams()
{
	int32 		cookie = 0;
	team_info	tInfo;
	BList		curList;
	int32		numItems = CountItems();
	int32		numCurItems = numItems;

	for (int32 i = 0; i < numItems; i++)
		curList.AddItem(ItemAt(i));

	while (get_next_team_info(&cookie, &tInfo) == B_NO_ERROR) {
		if (tInfo.team == B_SYSTEM_TEAM) 
			// filter out the kernel
			continue;

		BPath teamPath;
		get_path_for_team_info(&tInfo, &teamPath); 
		
		if (teamPath.InitCheck() != B_OK)
			continue;
		
		char signature[B_MIME_TYPE_LENGTH + 1] = "";
		ssize_t len = BNode(teamPath.Path()).ReadAttr("BEOS:APP_SIG", B_MIME_TYPE,
														0, signature, B_MIME_TYPE_LENGTH);
		if (len > B_OK) {
			signature[len] = 0;
			if ( (strcmp(signature, "application/x-vnd.Be-APPS") == 0) || 
				 (strcmp(signature, "application/x-vnd.Be-ROST") == 0) )
				// filter out the app_server and registrar
				continue;
		}
		
		bool foundTeam = false;

		for (int32 i = 0; i < numCurItems; i++) {
			if (((_TeamListItem_ *)curList.ItemAt(i))->Team() == tInfo.team) {
				curList.RemoveItem((int32)i);
				numCurItems--;
				foundTeam = true;
				break;
			}
		}

		if (foundTeam)
			continue;

		const char *kBootBeOS = "/boot/beos/system/";
		const char *kSystemServer = "/system/servers/";

		bool systemServer = (strncmp(teamPath.Path(), kBootBeOS, strlen(kBootBeOS)) == 0 || 
							 strncmp(teamPath.Path(), kSystemServer, strlen(kSystemServer)) == 0);
		bool crazy = false;
	
		for (int32 i = 0; i < *TeamMonitorWindow::sCrazyThreadsCount; i++) {
			if (TeamMonitorWindow::sCrazyThreads[(i * 3) + 2] == tInfo.team) {
				crazy = true;
				break;
			}
		}

		AddItem(new _TeamListItem_(&tInfo, &teamPath, systemServer, crazy), ((systemServer) && (!crazy)) ? CountItems() : 0);
		numItems++;
	}	

	for (int32 i = 0; i < numCurItems; i++) {
		team_id team = ((_TeamListItem_ *)curList.ItemAt(i))->Team();

		for (int32 j = 0; j < numItems; j++) {
			if (((_TeamListItem_ *)ItemAt(j))->Team() == team) {
				delete (RemoveItem((int32)j));
				break;
			}			
		}
	}
}


int32
_TeamListView_::MessageSender(
	void	*arg)
{
	_TeamListView_ *listView = (_TeamListView_ *)arg;

	while (listView->fSendMessages) {
		BWindow *window = listView->Window();
		if (window != NULL)
			window->PostMessage(msg_CheckTeams, listView);
		else
			return (B_NO_ERROR);

		snooze(500000);
	}

	return (B_NO_ERROR);
}

class TeamMonitorBox : public BBox {
public:
	TeamMonitorBox(BRect frame)
		:	BBox(frame, "box", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS
				| B_NAVIGABLE_JUMP | B_PULSE_NEEDED),
			startDesktopButton(NULL)
		{}
	virtual void Pulse();

private:
	BButton *startDesktopButton;
};

void 
TeamMonitorBox::Pulse()
{
	if (!static_cast<TeamMonitorWindow *>(Window())->IsEnabled()) {
		if (startDesktopButton) {
			startDesktopButton->RemoveSelf();
			startDesktopButton = NULL;
		}
		return;
	}

	bool trackerAndDeskbarRunning = be_roster->IsRunning(kTrackerSig)
		&& be_roster->IsRunning(kDeskbarSig);
	
	
	if (trackerAndDeskbarRunning) {
		if (startDesktopButton)
			startDesktopButton->SetEnabled(false);
	} else {
		if (!startDesktopButton) {
			BView *tmp = FindView("reboot");
			if (!tmp)
				return;
			
			BRect frame(tmp->Frame());
			startDesktopButton = new BButton(BRect(0, 0, 0, 0), B_EMPTY_STRING, 
				"Restart the Desktop", new BMessage(msg_Launch_trak));
			
			startDesktopButton->ResizeToPreferred();
			startDesktopButton->MoveTo(frame.right + 15, frame.top);
			AddChild(startDesktopButton);
			startDesktopButton->SetTarget(Window());
		}
		startDesktopButton->SetEnabled(true);
	}
}

TeamMonitorWindow::TeamMonitorWindow(
	uint8	*threadMonitor,
	uint32	threadMonitorSize)
		: BWindow(BRect(0.0, 0.0, kWindowWidth, kWindowHeight), 
				  "Team Monitor", 
				  B_TITLED_WINDOW_LOOK, 
				  B_NORMAL_WINDOW_FEEL, 
				  B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE)
{
	Lock();

	sThreadID = (thread_id *)threadMonitor;
	sThreadMonitor = threadMonitor + threadMonitorSize * sizeof(thread_id);
	sThreadMonitorSize = threadMonitorSize;
	sCrazyThreadsCount = (int32 *)(sThreadMonitor + sThreadMonitorSize);
	sCrazyThreads = sCrazyThreadsCount + 1;

	fDriver = B_ERROR;
	fTeamListView = NULL;

	BRect bounds = Bounds();

	// base
	BRect baseFrame = bounds;
	baseFrame.right += 1.0;
	baseFrame.bottom += 1.0;
	BBox *base = new TeamMonitorBox(baseFrame);
	AddChild(base);
	SetPulseRate(1000000);
	
	// first create all controls, in their tab order
	BRect infoFrame = bounds;
	infoFrame.InsetBy(12.0, 12.0);
	_TeamInfoView_ *infoView = new _TeamInfoView_(infoFrame);
	infoView->ResizeToPreferred();
	
	BRect listFrame = bounds;
	listFrame.InsetBy(12.0, 12.0);
	listFrame.right -= B_V_SCROLL_BAR_WIDTH;
	fTeamListView = new _TeamListView_(listFrame, infoView);
	
	BButton *killButton = new BButton(BRect(0.0, 0.0, 0.0, 0.0), B_EMPTY_STRING,
									  "Kill Application", new BMessage(msg_Kill));
	killButton->ResizeToPreferred();
	BButton *rebootButton = new BButton(BRect(0.0, 0.0, 0.0, 0.0), "reboot",
									  "Force Reboot", new BMessage(msg_Reboot));
	rebootButton->ResizeToPreferred();
	BButton *cancelButton = new BButton(BRect(0.0, 0.0, 0.0, 0.0), B_EMPTY_STRING, 
										"Cancel", new BMessage(msg_Disable));
	cancelButton->ResizeToPreferred();

	
	// cancel button
	BRect cancelBounds = cancelButton->Bounds();
	cancelButton->MoveTo(bounds.right - cancelBounds.Width() - 10.0, 
						 bounds.bottom - cancelBounds.Height() - 10.0);

	// reboot button
	BRect rebootBounds = rebootButton->Bounds();
	rebootButton->MoveTo(bounds.left + 10.0, 
					   bounds.bottom - rebootBounds.Height() - 10.0);

	// kill button
	BRect killBounds = killButton->Bounds();
	if (killBounds.Width() < rebootBounds.Width()) {
		killButton->ResizeTo(rebootBounds.Width(), rebootBounds.Height());
		killBounds = killButton->Bounds();
	}

	// info view
	infoView->SetKillButton(killButton);
	infoView->SetTeam(NULL, NULL);
	infoFrame = infoView->Frame();
	infoView->MoveTo(infoFrame.left, rebootButton->Frame().top - 12.0 - infoFrame.Height());
	infoFrame = infoView->Frame();

	killButton->MoveTo(bounds.left + 10.0,
					   infoFrame.top - killBounds.Height() - 9.0);
	
	// team list view
	listFrame.bottom = killButton->Frame().top - 12.0;
	fTeamListView->ResizeTo(listFrame.Width(), listFrame.Height());
	base->AddChild(new BScrollView(B_EMPTY_STRING, fTeamListView, B_FOLLOW_NONE, 0, false, true));	
	fTeamListView->MakeFocus();
	
	base->AddChild(killButton);
	base->AddChild(infoView);
	base->AddChild(rebootButton);
	base->AddChild(cancelButton);
	
	SetDefaultButton(cancelButton);
	killButton->SetTarget(fTeamListView);

	AddShortcut('T', B_SHIFT_KEY | B_CONTROL_KEY, new BMessage(msg_Launch_term));

	MoveTo(kDisabledLoc, kDisabledLoc);
	Show();
	PostMessage(msg_Raise_pri);

	Unlock();
}


void
TeamMonitorWindow::Enable(
	int	driver)
{
	memset(sThreadID, 0, sThreadMonitorSize * (sizeof(thread_id) + sizeof(uint8)) + B_PAGE_SIZE);

	thread_id	thisThread = find_thread(NULL);
	int32		teamCookie = 0;
	team_info	teamInfo;

	for(int32 i = 0; i < 4; i++)
	{
		while(get_next_team_info(&teamCookie, &teamInfo) == B_NO_ERROR) {
			int32		threadCookie = 0;
			thread_info	threadInfo;
	
			while (get_next_thread_info(teamInfo.team, &threadCookie, &threadInfo) == B_NO_ERROR) {
				if ((threadInfo.thread == thisThread) || (threadInfo.priority <= 10))
					continue;
	
				uint32 index = threadInfo.thread & (sThreadMonitorSize - 1);
				while(sThreadMonitor[index] && sThreadID[index] != threadInfo.thread)
					index = (index + 1) & (sThreadMonitorSize - 1);

				sThreadID[index] = threadInfo.thread;
				sThreadMonitor[index] = ((threadInfo.kernel_time + threadInfo.user_time) & 0xFFFFFFFF) / 2048;		
				if (sThreadMonitor[index] == 0)
					sThreadMonitor[index] = 1;
			}
		}	

		// 1/8 second
		snooze(125000);
	
		teamCookie = 0;
		while (get_next_team_info(&teamCookie, &teamInfo) == B_NO_ERROR) {
			int32		threadCookie = 0;
			thread_info	threadInfo;
	
			while (get_next_thread_info(teamInfo.team, &threadCookie, &threadInfo) == B_NO_ERROR) {
				if ((threadInfo.thread == thisThread) || (threadInfo.priority <= 10))
					continue;
	
				uint32 index = threadInfo.thread & (sThreadMonitorSize - 1);
				while(sThreadMonitor[index] && sThreadID[index] != threadInfo.thread)
					index = (index + 1) & (sThreadMonitorSize - 1);

				if(sThreadMonitor[index] && sThreadID[index] != threadInfo.thread)
					continue;

				uint8	oldTime = sThreadMonitor[index];
				if (oldTime == 0)
					continue;
	
				uint8 newTime = ((threadInfo.kernel_time + threadInfo.user_time) & 0xFFFFFFFF) / 2048;
				if (newTime == 0)
					newTime = 1;
	
				uint8 diffTime = newTime - oldTime;
				if (diffTime > 25) {
					set_thread_priority(threadInfo.thread, 1);
	
					if (((((*sCrazyThreadsCount) + 1) * 3 * sizeof(int32)) + sizeof(*sCrazyThreadsCount)) < B_PAGE_SIZE) {
						int32 index = (*sCrazyThreadsCount) * 3;
						sCrazyThreads[index] = threadInfo.thread;
						sCrazyThreads[index + 1] = threadInfo.priority;
						sCrazyThreads[index + 2] = threadInfo.team;
						(*sCrazyThreadsCount)++;
					}
				}
			}
		}		

		// 1/8 second
		snooze(125000);
	}

	Lock();

	fDriver = driver;
	sEnabled = true;

	fTeamListView->Enable();	
	SetFeel(B_MODAL_ALL_WINDOW_FEEL);

	// center window
	BRect screenFrame = BScreen().Frame();
	MoveTo((screenFrame.Width() - kWindowWidth) / 2,
		   (screenFrame.Height() - kWindowHeight) / 3);

	PostMessage(msg_Ping);

	Unlock();
}


void
TeamMonitorWindow::Disable()
{
	Lock();

	// "hide" the window
	MoveTo(kDisabledLoc, kDisabledLoc);
	SetFeel(B_NORMAL_WINDOW_FEEL);
	fTeamListView->Disable();

	fDriver = B_ERROR;
	sEnabled = false;

	Unlock();

	for (int32 i = 0; i < *sCrazyThreadsCount; i++) {
		int32 index = i * 3;
		set_thread_priority(sCrazyThreads[index], sCrazyThreads[index + 1]);
	}
}	


bool
TeamMonitorWindow::IsEnabled() const
{
	return (sEnabled);
}


void
TeamMonitorWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	int32 key;
	if (
		(message->what == B_KEY_DOWN) &&
		(message->FindInt32("raw_char", &key)== B_OK) &&
		(key == B_ESCAPE)
	) {
		Disable();
	} else {
		BWindow::DispatchMessage(message, handler);
	}
}


void
TeamMonitorWindow::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case msg_Raise_pri :
			set_thread_priority(find_thread(0), 120);
			break;

		case msg_Ping:
			{
			// this used to talk with the keyboard driver to abort
			// a control-alt-del reset; that is no longer needed.
			}
			break;

		case msg_Reboot :
			_kshutdown_(true);	// reboot!
			break;	// uh, what?

		case msg_Disable:
			if (IsEnabled())
				Disable();
			break;

		case msg_Launch_term:
			BRoster().Launch(kTerminalSig);
			break;

		case msg_Launch_trak:
			if (!be_roster->IsRunning(kTrackerSig))
				BRoster().Launch(kTrackerSig);
			if (!be_roster->IsRunning(kDeskbarSig))
				BRoster().Launch(kDeskbarSig);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

bool
TeamMonitorWindow::QuitRequested()
{
	if(IsEnabled()) {
		Disable();
	}

	return false;
}


