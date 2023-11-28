#include <Alert.h>
#include "Util.h"
#include "InstallerType.h"
#include "MyDebug.h"
#include <string.h>

// for very simple error message
long doError(const char *msg)
{
	return doError(msg,NULL,"OK");
}

long doError(const char *format, const char *subtext)
{
	return doError(format,subtext,"OK");
}

long doFatal(const char *msg, const char *subtext)
{
#if (SEA_INSTALLER)
	return doError(msg,subtext,"Quit");
#else
	return doError(msg,subtext,"Cancel");
#endif
}

long doError(const char *format,
			 const char *str,
			 const char *button1,
			 const char *button2,
			 const char *button3)
{
	long		result;
	char		*msg;
	
	if (str) {
	 	msg = new char[strlen(format) + strlen(str) + 2];
		sprintf(msg,format,str);
	}
	else {
		// cast away const
		msg = (char *)format;
	}
	BAlert *errorAlert = new BAlert(B_EMPTY_STRING,msg,button1,button2,button3,
							B_WIDTH_FROM_WIDEST,B_WARNING_ALERT);
	PositionWindow(errorAlert,0.5,0.25);
	result = errorAlert->Go();
	if (str)
		delete[] msg;

	return result;
}
