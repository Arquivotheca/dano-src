/*	$Id: HDlogView.h,v 1.1 1998/11/14 14:20:52 maarten Exp $

	HDlogView.h, Copyright Hekkelman Programmatuur
	Created: 11/27/97 22:06:16 by Maarten Hekkelman

	$Log: HDlogView.h,v $
	Revision 1.1  1998/11/14 14:20:52  maarten
	Changed the names of almost all the library files
	
	Revision 1.4  1998/10/06 18:40:51  maarten
	Changes caused by porting to MacOS
	
	Revision 1.3  1998/01/16 14:13:55  hekkel
	some changes
	
	Revision 1.2  1998/01/09 19:21:58  hekkel
	fixes for intel release

	Revision 1.1  1997/12/04 20:09:45  hekkel
	added background view
	
*/

#ifndef HDLOGVIEW_H
#define HDLOGVIEW_H

#include <View.h>

#include <vector>
using std::vector;

class HDlogView : public BView {
public:
		HDlogView(BRect frame, const char *name);
		~HDlogView();
		
virtual	void Draw(BRect update);

		void AddMyLine(BRect r);

private:
		vector<BRect> fLines;
};

#endif
