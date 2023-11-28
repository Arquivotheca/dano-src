#ifndef	POSTSCRIPT
#define	POSTSCRIPT

//------------------------------------------------------------------

#include <Point.h>
#include <Looper.h>

#include "ppd_control.h"
#include "PSGenerator.h"

class BBitmap;
class BFilePanel;

class	TPrintDrv {
 public:
				TPrintDrv();
				~TPrintDrv();

	int			ReadPPD(const char* ppd_file);
	BMessage	*take_job(BFile *the_file, BNode *printer_file, BMessage *msg);
	BMessage	*page_setup(BMessage *msg, char *pr_name);
	BMessage	*print_setup(BMessage *msg, char *pr_name);
	BMessage	*get_default_settings(BNode *printer);

	BMessage		*setup;
	long			ffd;
	BFilePanel 		*save_panel;
	BEntry			*entry;

 private:

	long		do_page_setup(BMessage *msg, char *pr_name);
	long		do_print_setup(BMessage *msg, char *pr_name);

	BBitmap		*a_bitmap;
	BView		*a_view;
	BList		*page_index;


};


#endif
