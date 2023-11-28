
#ifndef _PDF_ALERT_H_
#define _PDF_ALERT_H_

#include <SupportDefs.h>

#include <Binder.h>

class PDFAlert {
	public:
									PDFAlert();
									~PDFAlert();
		void						ThrowAlert(status_t alert);
		void						ResetAlerts();
	private:
		uint32						fAlertShown;
		BinderNode::property		fNotifier;
};

#endif
