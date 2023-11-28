#define DEBUG 0

#include "Log.h"
#include <FindDirectory.h>
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
	BPath path;
	// find settings dir; create it if needed
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path, true) == B_OK)
	{
		path.Append("SoftwareValet");
		SetFileLoc(path.Path());
	}
	
	r = 0;
	w = 0;
}

Log::Log(const char *logfile_loc)
	:	file_loc(0)
{
	SetFileLoc(logfile_loc);
	
	r = 0;
	w = 0;
}

Log::~Log(void)
{
	free(file_loc);
}

status_t Log::PostLogEvent(BMessage *msg)
{
	time_t timet = time(NULL);
	char *timestr = new char[BUFMAX];
	int errorchk = 0;
		
	PRINT(("r: %d, w: %d at the beg. of PostLogEvent\n", r, w));
	
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
		errorchk = fseek(fileptr, r, SEEK_SET);		// go to read pt
		if (errorchk !=0) return(NULL);
		ssize_t size_of_next;
		int result = fread(&size_of_next, sizeof(size_of_next), 1, fileptr);
		if (result < 1) return(NULL);
		
		BMessage *nextmsg = new BMessage;	// creat msg
		char *next_flat_buf = new char[size_of_next];
		errorchk = fread(next_flat_buf, size_of_next, 1, fileptr);
		if (errorchk != 1) goto errorjmp;
		r = ftell(fileptr);
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

