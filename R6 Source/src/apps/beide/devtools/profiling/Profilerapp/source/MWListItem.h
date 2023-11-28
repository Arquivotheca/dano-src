#include "ProfilerApp.h"

const rgb_color GreyColor ={216,216,216,255};
const rgb_color Black ={0,  0,  0,		255};
const rgb_color White ={255,255,255,255};
const rgb_color DarkGrey =	{220,220,220,255};
const rgb_color BeLightShadow =	{194,194,194,255};
const rgb_color Green =	{0,255,0,255};



long double bcalled,busec,buser,bkernel,bclock,bname=0;

struct ProfStruct
{
long double called,usec,user,kernel,clock;
char scalled[15],susec[15],suser[15],skernel[15],sclock[15];
char methodname[240];
};


class MWListItem:public BListItem
{
	public:
		MWListItem( const char name[80], const long double called, const long double usec, const long double user,
			const long double kernel, const long double clock, uint32 outlineLevel = 0, bool expanded = true);
		~MWListItem(){};
		virtual void DrawItem(BView *owner, BRect itemRect, bool complete=false);
		friend void read_in_data(char *filename, BScrollView *tScrollView);
		friend int compare_name(MWListItem **firstArg, MWListItem **MWsecondArg);
		friend int rcompare_name(MWListItem **firstArg, MWListItem **MWsecondArg);
		friend int compare_called(MWListItem **firstArg, MWListItem **MWsecondArg);
		friend int rcompare_called(MWListItem **firstArg, MWListItem **MWsecondArg);
		friend int compare_usec(MWListItem **firstArg, MWListItem **MWsecondArg);
		friend int rcompare_usec(MWListItem **firstArg, MWListItem **MWsecondArg);
		friend int compare_user(MWListItem **firstArg, MWListItem **MWsecondArg);
		friend int rcompare_user(MWListItem **firstArg, MWListItem **MWsecondArg);
		friend int compare_kernel(MWListItem **firstArg, MWListItem **MWsecondArg);
		friend int rcompare_kernel(MWListItem **firstArg, MWListItem **MWsecondArg);
		friend int compare_clock(MWListItem **firstArg, MWListItem **MWsecondArg);		
		friend int rcompare_clock(MWListItem **firstArg, MWListItem **MWsecondArg);	
		char *MakeString( int *total);
	private:
		uint32 itemlevel;
		struct ProfStruct  profData;
};

