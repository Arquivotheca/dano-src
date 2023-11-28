
#include <unistd.h>
#include <Alert.h>

#include "ValidApp.h"
#include "settings.h"


static void
post_validate()
{
	int64 show_alert = get_setting_value("validate.endalert", 1);
	if (g_return_code == 0) {
		BString validate_script = ValidApp::s_current_directory;
		validate_script += "/validate.ok";
		system(validate_script.String());
		//	alert the tester
		fclose(m_results);
		m_results = stderr;
		sync();
		// normally don't show alert - continue into prepare (or quit)
		if (show_alert)
		{
			BAlert* alert = new BAlert("", "This machine has been validated and is now prepared for customer use.", "OK");
			SetLargeAlertFont(alert);
			alert->Go();
		}
	}
	else if (show_alert) {
		BAlert* alert = new BAlert("", "This machine has not been prepared for customer use.", "OK");
		SetLargeAlertFont(alert);
		alert->Go();
	}
}

bool g_failed = false;
int32 g_return_code = 1;

int
main()
{
	fprintf(stderr, "Validate copyright 2000 Be, Incorporated.\n");
	fprintf(stderr, "MD5 checksum by Eric Young from SSLeay.\n");
	ValidApp* myApp = new ValidApp();
	myApp->Run();
	post_validate();
	StatusWindow::Close();

	delete myApp;
	fprintf(stderr, "Returning %ld from main\n", g_return_code);
	return g_return_code;
}
