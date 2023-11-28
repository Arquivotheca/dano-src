/*
	MailDebug.h
*/
#ifndef MAIL_DEBUG_H
#define MAIL_DEBUG_H

#define DEBUG 1
#include <Debug.h>
#include <Locker.h>
#include <String.h>
#include <StringBuffer.h>
#include <StreamIO.h>

#if DEBUG
	#define MDB(x) MailDebug md(__PRETTY_FUNCTION__, __FUNCTION__)
	#define DB(x) x
#else
	#define MDB(x) (void)0
	#define DB(x) (void)0
#endif
 
extern BDataIO& BDebugOutput;

class MailDebug {
	public:
								MailDebug(const char *className, const char *functionName);
								~MailDebug();

		void					Indent();
		void					Print(const char *str, ...);

		static void				SPrint(const char *str, ...);
		static void				SetDebugOutput(const char *str);
				
	private:
		StringBuffer fClass;
		BString fName;
		thread_id fThread;
		BDataIO *fOutput;
		
		static int32 fIndent;
		static BLocker fPrintLock;
		static BDataIO *fDebugOutput;
};

#endif
// End of MailDebug.h
