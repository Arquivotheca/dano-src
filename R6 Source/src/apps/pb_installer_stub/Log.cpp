#define DEBUG 0

#include "Log.h"
#include <time.h>
#include <Debug.h>
#include <Path.h>
#include <string.h>

// #include "LogPrefsView.h"
long CreatePathAt(const BDirectory *dir, const char *path, BEntry *final);


const char *Log::logFileName = "SoftwareValet_log";


Log::Log()
	:	file_loc(0)
{
	SetFileLoc("/boot/home/config/settings");
	
	r = new long;
	w = new long;
	*r = 0;
	*w = 0;
}

Log::Log(const char *logfile_loc)
	:	file_loc(0)
{
	SetFileLoc(logfile_loc);
	
	r = new long;
	w = new long;
	*r = 0;
	*w = 0;
}

Log::~Log(void)
{
	free(file_loc);
	delete r;
	delete w;
}

status_t Log::PostLogEvent(BMessage *msg)
{
	time_t timet = time(NULL);
	char *timestr = new char[BUFMAX];
	int errorchk = 0;
		
	PRINT(("r: %d, w: %d at the beg. of PostLogEvent\n", *r, *w));
	
	strftime(timestr, BUFMAX, "%a %b %d %I:%M:%S %p", localtime(&timet));
	msg->AddString("timestamp", timestr);
	
	ssize_t msgsize = msg->FlattenedSize();
	char *flatbuf = new char[msgsize];
	msg->Flatten(flatbuf, msgsize);
	
	fileptr = fopen(file_loc, "a+");
	if (fileptr==NULL) goto errorjmp;
	errorchk = fseek(fileptr, 0, SEEK_END);
	if (errorchk != 0) goto errorjmp;
	errorchk = fwrite(&msgsize, sizeof(msgsize), 1, fileptr);
	if (errorchk != 1) goto errorjmp;
	errorchk = fwrite(flatbuf, msgsize, 1, fileptr);
	if (errorchk != 1) goto errorjmp;
	errorchk = fclose(fileptr);
	if (errorchk == EOF) goto errorjmp;
	
	delete flatbuf;
	delete timestr;
	return(B_NO_ERROR);
errorjmp:
	delete flatbuf;
	delete timestr;
	
	
	return(B_ERROR);
}

BMessage *Log::GetNextEntry(void)
{
	int errorchk = 0;
	while(true) {
		
		fileptr = fopen(file_loc, "r");	// check file
		if (fileptr == NULL) {
			PRINT(("GetNextEntry opened bad file\n"));
			return(NULL);
		}
		errorchk = fseek(fileptr, *r, SEEK_SET);		// go to read pt
		if (errorchk !=0) return(NULL);
		ssize_t size_of_next;
		int result = fread(&size_of_next, sizeof(size_of_next), 1, fileptr);
		if (result < 1) return(NULL);
		
		BMessage *nextmsg = new BMessage;	// creat msg
		char *next_flat_buf = new char[size_of_next];
		errorchk = fread(next_flat_buf, size_of_next, 1, fileptr);
		if (errorchk != 1) goto errorjmp;
		*r = ftell(fileptr);
		errorchk = fclose(fileptr);
		if (errorchk == EOF) goto errorjmp;
		
//		LogEntry *nextEntry = new LogEntry;//create log entry
		nextmsg->Unflatten(next_flat_buf);
//		nextEntry->SetEntry(nextmsg);
		delete (next_flat_buf);
		return(nextmsg);
errorjmp:	
		delete (next_flat_buf);
		return(NULL);
	}
	return(NULL);
}


void Log::SetFileLoc(const char *fullpath) 
{
	BPath	p(fullpath);
	p.Append(logFileName);

	free(file_loc);
	file_loc = strdup(p.Path());
}

void Log::EraseLogFile(void)
{
	fileptr = fopen(file_loc, "w");
}

/*status_t	Log::MakeEmailFile(char *message)
{
	// this function uses a mix of POSIX and BeOS file calls
	// fprintf is a handy function!

	// make a file
	const char *path = "/boot/home/config/settings/StarCode/ValetMsg";
	char *generic = "The SoftwareValet performed an automated software management service for you.";
	if (message == NULL) message = generic;
	
	FILE	*f = fopen(path,"w+");

	if (!f)
		return B_ERROR;

	// make mime header

	// make a date string
	time_t timet = time(NULL);
	char *datestring = new char[50];
	struct tm *timestruct;
	timestruct = localtime(&timet);
	strftime(datestring, 50, "%a, %d %b %Y %H:%M:%S %Z", timestruct);
	fprintf(f,"Date: %s\r\n",datestring);
	//fprintf(f,"Date: Mon, 28 Jul 1997 11:38 PDT\r\n");
	fprintf(f,"MIME-Version: 1.0\r\n");
	fprintf(f,"X-Mailer: SoftwareValet 1.0.1 on Be OS\r\n");
	fprintf(f,"Subject: SoftwareValet activity\r\n");
	fprintf(f,"Sender: SoftwareValet\r\n");
	fprintf(f,"From: SoftwareValet\r\n");
	fprintf(f,"Content-Type: text/plain\r\n");
	// etc...

	fprintf(f,"\r\n\r\n");
	//msg
	fprintf(f,"%s\r\n", message);
	fprintf(f,"\r\nThe SoftwareValet log tracks downloads, installations, registrations, trolling for updates, and more. The Valet has performed an automated software management task for you; consult the log for details.\r\n");
	fprintf(f,"\r\nTo display the log, launch the SoftwareValet and do one of the following:\r\n\r\n");
	fprintf(f,"1) Choose \"Manage\" from the palette and then choose \"Display Log\"\r\n");
	fprintf(f,"2) Choose \"Configure\" from the palette and access the log from the \"Download\" settings\r\n");
	fprintf(f,"\r\n\r\n-------------------------------------------------------------------------------------------------------------------------------\r\n");
	fprintf(f,"Copyright 1997 StarCode Software, Inc.\r\n");
	fprintf(f,"Providing software distribution and management tools for the Be operating system\r\n");
	fprintf(f,"support@starcode.com\r\n");
	fclose(f);

	{
		BFile	bf(path,O_RDWR);
	
		// check for error
		if (bf.InitCheck() != B_NO_ERROR)
			return B_ERROR;
	
		bf.Lock();
	
		// all of the attribute fields
		const int numFields = 5;
		struct field {
			char *attr;
			char *value;
		}	eFields[] ={
					{"MAIL:name","SoftwareValet"},
					{"MAIL:from","SoftwareValet"},
					{"MAIL:subject","SoftwareValet activity"},
					{"BEOS:TYPE", "text/x-email"},
					{"MAIL:status", "New"}};
	
		for (int i = 0; i < numFields; i++) {
		bf.RemoveAttr(eFields[i].attr);
		bf.WriteAttr(eFields[i].attr,B_STRING_TYPE,0,eFields[i].value,
					strlen(eFields[i].value) + 1);
		}
		bf.Unlock();
	}

	//BFile	bf(path, O_RDWR);

	//bf.Lock();
	//bf.RemoveAttr("MAIL:status");
	//bf.WriteAttr("MAIL:status",B_STRING_TYPE,0,"New",4);
	//bf.Unlock();
	
	// launch notifier
	// check this signature in filetypes
	
	return be_roster->Launch("application/x-adam-alert");
}*/
