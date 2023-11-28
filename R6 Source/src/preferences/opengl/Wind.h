#ifndef __WIND_H__
#define __WIND_H__

#include <PopUpMenu.h>
#include <ListView.h>
#include <MenuField.h>

class ResList : public BListView
{
public:
	ResList( BRect r );
	virtual ~ResList();
	
	virtual void SelectionChanged();
	
private:
	class ListItem : public BStringItem
	{
	public:
		ListItem( const char *name, void *mode )
			: BStringItem( name )
			{
				data = mode;
			}
		virtual ~ListItem() {}
		
		void *data;
	};
		
};


class MainWindow : public BWindow
{
public:
	MainWindow();
	virtual ~MainWindow();

	
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage*);

	void SetToDevice( int deviceID );
	void SetToMode( int w, int h );
	

	BMenuField *deviceField;
	BPopUpMenu *deviceMenu;
	ResList *resList;
	BMenuField *hzField;
	BPopUpMenu *hzMenu;
	BCheckBox *resEnable;

	BMenuField *monField;
	BPopUpMenu *monMenu;
	
	BMenuItem *monItems[4];
};




#endif

