

#include "PDFAlert.h"

#include <Binder.h>

enum {
	PDF_UNSUPPORTED			= 	(1 << 0),
	PDF_NO_RANDOM_ACCESS	=	(1 << 1),
	PDF_FILE_ERROR			=	(1 << 2),
	PDF_NO_INIT				= 	(1 << 3)
};


PDFAlert::PDFAlert() :
	fAlertShown(0)
{
	fNotifier = BinderNode::Root() ["service"] ["notification"] ["pdf"];
}


PDFAlert::~PDFAlert()
{
}

void 
PDFAlert::ThrowAlert(status_t alert)
{
	const char *string = NULL;
	switch(alert) {
		case B_UNSUPPORTED: {
			if ((fAlertShown & PDF_UNSUPPORTED)) ;
			else {
				string = "B_UNSUPPORTED";
				fAlertShown |= PDF_UNSUPPORTED;
			}
			break;
		}
		
		case B_NO_RANDOM_ACCESS: {
			if (! (fAlertShown & PDF_NO_RANDOM_ACCESS)) {
				string = "B_NO_RANDOM_ACCESS";
				fAlertShown |= PDF_NO_RANDOM_ACCESS;
			}
			break;
		}

		case B_IO_ERROR: {
			string = "B_IO_ERROR";
			break;
		}
		
		case B_FILE_ERROR: {
			if (! (fAlertShown & PDF_FILE_ERROR)) {
				string = "B_FILE_ERROR";
				fAlertShown |= PDF_FILE_ERROR;
			}
			break;
		}

		case B_NO_INIT: {
			if (! (fAlertShown & PDF_NO_INIT)) {
				string = "B_NO_INIT";
				fAlertShown |= PDF_NO_INIT;
			}
			break;
		}
		
		case B_NO_PRINTER: {
			string = "B_NO_PRINTER";
			break;
		}

	}


//	printf("ThrowAlert(%lx): %s\n", alert, string);

	if (string) {
		BinderNode::property_list args;
		BinderNode::property code(string);
		args.AddItem(&code);
		BinderNode::property priority(0);
		args.AddItem(&priority);
	
		fNotifier["Post"](args);
	}
}

void 
PDFAlert::ResetAlerts()
{
	fAlertShown = 0L;
}

