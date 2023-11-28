#include <Be.h>
#include "Util.h"

const rgb_color light_gray_background = {230,230,230,0};
const rgb_color disabled_color = {127,127,127,0};

// for very simple error message
long doError(const char *msg, status_t errcode)
{
	return doError(msg,NULL,"OK",NULL,NULL,errcode);
}

long doError(const char *format, const char *subtext, status_t errcode)
{
	return doError(format,subtext,"OK",NULL,NULL,errcode);
}

long doError(const char *format,
			 const char *str,
			 const char *button1,
			 const char *button2,
			 const char *button3,
			 status_t	errcode)
{
	long		result;
	char		*msg;
	const char		*errmsg = B_EMPTY_STRING;
	
	if (errcode != 0)
		errmsg = strerror(errcode);
	
	if (str) {
	 	msg = new char[strlen(format) + strlen(str) + strlen(errmsg) + 2];
		sprintf(msg,format,str,errmsg);
	}
	else if (errmsg && *errmsg) {
		msg = new char[strlen(format) + strlen(errmsg) + 2];
		sprintf(msg,format,errmsg);
	}
	else {
		// cast away const
		msg = (char *)format;
	}
	BAlert *errorAlert = new BAlert(B_EMPTY_STRING,msg,button1,button2,button3,
							B_WIDTH_AS_USUAL,B_WARNING_ALERT);
	PositionWindow(errorAlert,0.5,0.25);
	result = errorAlert->Go();
	
	if (str)
		delete[] msg;
		
	return result;
}


