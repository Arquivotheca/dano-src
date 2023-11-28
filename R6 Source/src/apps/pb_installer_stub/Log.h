#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <Message.h>

#define BUFMAX		256
#define MSGMAX		512

class Log {
	public:
						Log();
						Log(const char *logfile_loc);
						~Log();
		status_t 		PostLogEvent(BMessage *msg);
		BMessage 		*GetNextEntry(void);		
		void 			SetFileLoc(const char *fullpath);
		void 			EraseLogFile(void);


		enum {	LOG_LISTENER_LAUNCH,
				LOG_LISTENER_QUIT,
				LOG_TROLL_BEGIN,
				LOG_TROLL_END, 
				LOG_VALET_LAUNCH,
				LOG_VALET_QUIT,
// Actions before VALET_QUIT should not be associated with a package,
// those after should all refer to a package.
				LOG_INSTALL, 
				LOG_UNINSTALL,
				LOG_REGISTER,
				LOG_DOWNLOAD, 
				LOG_RESUME_DOWNLOAD,
				LOG_UPDATE_FOUND,
				LOG_JAEGAR_CONNECT,
				LOG_MANUAL_UPDATE };
		
	private:
		static const char *logFileName;
		char *file_loc;
		long *r;
		long *w;
		FILE *fileptr;
};

#endif
