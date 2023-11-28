#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <FindDirectory.h>
#include <sys/file.h>
#include <sys/param.h>
#include <unistd.h>
#include <string.h>
#include <priv_syscalls.h>

#include <Application.h>
#include <Alert.h>
#include <MessageRunner.h>
#include <Roster.h>
#include <TextView.h>
#include <kernel/image.h>

#define TICK_ALERT 'taTT'

class TTimedAlert : public BAlert
{
public:
	TTimedAlert(const char *title,
				const char *text,
				const char *button1,
				const char *button2 = NULL,
				const char *button3 = NULL,
				button_width width = B_WIDTH_AS_USUAL,
				alert_type type = B_INFO_ALERT);
	virtual	void		Show();
	virtual	void		MessageReceived(BMessage *msg);

private:
	char		sprintf_string[1024];
};

TTimedAlert::TTimedAlert(const char *title, const char *text, const char *button1, const char *button2, const char *button3, button_width width, alert_type type)
	: BAlert(title, text, button1, button2, button3, width, type)
{
	strcpy(sprintf_string, text);
}

void TTimedAlert::Show()
{
	char	tmp[1024];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char timestr[30];

	strftime(timestr, 29, "%I:%M %p", &tm);
	char *timep = (timestr[0] == '0' ? &timestr[1] : &timestr[0]);
	sprintf(tmp, sprintf_string, timep);
	TextView()->SetText(tmp);
	BAlert::Show();
	new BMessageRunner(this, new BMessage(TICK_ALERT), 10000000LL);
}

void TTimedAlert::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
	case TICK_ALERT : 
		{
			char	tmp[1024];
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			char timestr[30];

			strftime(timestr, 29, "%I:%M %p", &tm);
			char *timep = (timestr[0] == '0' ? &timestr[1] : &timestr[0]);
			sprintf(tmp, sprintf_string, timep);
			TextView()->SetText(tmp);
		}
		break;

	default :
		BAlert::MessageReceived(msg);
		break;
	}
}

int
main()
{
	time_t t;
	struct tm tm;
	int isdst, prev_isdst;
	char fname[MAXPATHLEN+1];
	int fd = 0;
	char c;
	rtc_info info;

	if (_kget_rtc_info_(&info) != 0) {
		fprintf(stderr, "couldn't get RTC information\n");
		exit(-1);
	}

	if (info.is_gmt) {
		exit(0);
	}
	
	t = time(NULL);
	tm = *localtime(&t);

	isdst = tm.tm_isdst;

	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, TRUE, fname, MAXPATHLEN);
	strcat(fname, "/time_dststatus");
	if (((fd = open(fname, O_RDWR)) < 0) || (read(fd, &c, 1) != 1)) {
		if (fd)
			close(fd);
		prev_isdst = -1;
	} else {
		prev_isdst = (int) (c - '0');
	}

	if (prev_isdst == isdst)
		exit(0);
	else if (prev_isdst == -1) {
		if ((fd = open(fname, O_CREAT|O_TRUNC|O_RDWR, 0644)) < 0) {
			perror("couldn't open dst status settings file");
			exit(1);
		}
	} else if (prev_isdst != isdst) {
		BApplication *app = new BApplication("application/x-vnd.Be-cmd-dstconfig");
		char command[1024];
		char timestr[30];

		strftime(timestr, 29, "%I:%M %p", &tm);
		sprintf(command, "Attention!\n\nBecause of the switch %s daylight saving time, your computer's clock may be an hour off. Currently, your computer thinks it is %%s.\n\nIs this the correct time?", 
				(isdst ? "to" : "from")); 
		
		TTimedAlert *alrt1 = 
			new TTimedAlert("Alert", command, "Ask me later", "Yes", "No", 
							B_WIDTH_AS_USUAL, B_INFO_ALERT);
		alrt1->SetWorkspaces(B_ALL_WORKSPACES);
		alrt1->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
		int32 result = alrt1->Go();
		
		if (result == 0) { 
			goto done;
		} else if (result == 2) {
			BAlert *alrt2 = 
				new BAlert("Alert", "Would you like to set the clock using the Time and Date preference utility?", "No", "Yes");
			alrt2->SetWorkspaces(B_ALL_WORKSPACES);
			alrt2->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
			result = alrt2->Go();
			
			if (result == 1)
				BRoster().Launch("application/x-vnd.Be-TIME");
		}

		if (lseek(fd, 0, SEEK_SET) < 0) {
			perror("lseek");
			exit(3);
		}
	}

	c = (char) isdst + '0';
	if (write(fd, &c, 1) < 0) {
		perror("couldn't write to dst status settings file");
		exit(2);
	}
	ftruncate(fd, 1);
 done:
	close(fd);
}
