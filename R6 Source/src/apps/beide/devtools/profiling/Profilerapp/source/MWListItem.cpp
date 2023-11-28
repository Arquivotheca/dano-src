#include "MWListItem.h"


MWListItem::MWListItem( const char  *name, const long double called, const long double usec, const long double user,const long double kernel,
			const long double clock, uint32 outlineLevel , bool expanded)
			:BListItem(outlineLevel,expanded)
{
	/** Put the data read from the profile file into the struct **/
	strncpy(profData.methodname, name,strlen(name)); 
	profData.called=called;
	sprintf(profData.scalled,"%1.0Lf",called);
	profData.usec=usec;
	sprintf(profData.susec,"%1.2Lf",usec);
	profData.user=user;
	sprintf(profData.suser,"%1.0Lf",user);
	profData.kernel=kernel;
	sprintf(profData.skernel,"%1.0Lf",kernel);
	profData.clock=clock;
	sprintf(profData.sclock,"%1.0Lf",profData.clock);
}



void
MWListItem::DrawItem(BView *owner,BRect bounds,	bool /*complete*/)
{
	rgb_color viewColor=White;
	bool		selected = IsSelected();
	BRect MWRect;
	BRegion MWRegion;
	
	if (selected) /* Draw the ListItem as selected */
	{
		owner->SetLowColor(GreyColor);
		owner->SetDrawingMode(B_OP_COPY);
		owner->FillRect(bounds, B_SOLID_LOW);
		owner->SetLowColor(viewColor);
		owner->SetDrawingMode(B_OP_OVER);// uses background color for antialiasing
	}
	else  /* Unselect the ListItem */ 
	{
		owner->SetDrawingMode(B_OP_COPY);
		owner->SetHighColor(viewColor); 
		owner->FillRect(bounds); 
		owner->SetHighColor(Black); 
	}
	/** This is to have the columns set, we will have resizable columns later */
	const float one=2,two=212,three=284,four=380,five=456,six=550;
	
	MWRect.Set(bounds.left,bounds.top,170,bounds.bottom-2);
	MWRegion.Set(MWRect);
	owner->MovePenTo(BPoint(bounds.left + one, bounds.bottom - 4));
	owner->ConstrainClippingRegion(&MWRegion);
	owner->DrawString(profData.methodname);
	owner->ConstrainClippingRegion(NULL);

	owner->MovePenTo(BPoint(bounds.left + two, bounds.bottom - 4));
	owner->DrawString(profData.scalled);
	
	owner->MovePenTo(BPoint(bounds.left + three, bounds.bottom - 4));
	owner->DrawString(profData.susec);
	
	owner->MovePenTo(BPoint(bounds.left + four, bounds.bottom - 4));
	owner->DrawString(profData.suser);
	
	owner->MovePenTo(BPoint(bounds.left + five, bounds.bottom - 4));
	owner->DrawString(profData.skernel);
	
	owner->MovePenTo(BPoint(bounds.left + six, bounds.bottom - 4));
	owner->DrawString(profData.sclock);
}


/** This reads in the items in a row and sticks them into one array and passes
	it back **/
char *MWListItem::MakeString( int *total)
{
	char *cpArr;
	int method,called,usec,user,kernel, clock;
	char tab='\t';
	char endol='\n';
	char term='\0';

	method=strlen(profData.methodname);
	called=strlen(profData.scalled);
	usec=strlen(profData.susec);
	user=strlen(profData.suser);
	kernel=strlen(profData.skernel);
	clock=strlen(profData.sclock);
	*total=method+called+usec+user+kernel+clock+6;
		
	cpArr=new char[*total+1];

	memset(cpArr,NULL,*total);
	sprintf(cpArr,"%s",profData.methodname);
	cpArr[method]=tab;

	sprintf(&cpArr[method+1],"%s",profData.scalled);
	cpArr[method+called+1]=tab;
	sprintf(&cpArr[method+called+2],"%s",profData.susec);
	cpArr[method+called+usec+2]=tab;
	sprintf(&cpArr[method+called+usec+3],"%s",profData.suser);
	cpArr[method+called+usec+user+3]=tab;

	sprintf(&cpArr[method+called+usec+user+4],"%s",profData.skernel);
	cpArr[method+called+usec+user+kernel+4]=tab;
	sprintf(&cpArr[method+called+usec+user+kernel+5],"%s",profData.sclock);
	cpArr[method+called+usec+user+kernel+clock+5]=endol;
	cpArr[method+called+usec+user+kernel+clock+6]=term;
			
	return cpArr;
}

/*** Below are the sort functions!!!!!!  ***/

int compare_name(MWListItem **firstArg, MWListItem **secondArg)
{
	char buf[40]="\0";
	char bufx[40]="\0";
	int i;
	
	for (i=0;(*firstArg)->profData.methodname[i]!='(';i++)
		buf[i]=(*firstArg)->profData.methodname[i];

	for (i=0;(*secondArg)->profData.methodname[i]!='(';i++)
		bufx[i]=(*secondArg)->profData.methodname[i];

		return strcmp(buf,bufx);
}

int rcompare_name(MWListItem **firstArg, MWListItem **secondArg)
{
	char buf[40]="\0";
	char bufx[40]="\0";
	int i;
	
	for (i=0;(*firstArg)->profData.methodname[i]!='(';i++)
		buf[i]=(*firstArg)->profData.methodname[i];

	for (i=0;(*secondArg)->profData.methodname[i]!='(';i++)
		bufx[i]=(*secondArg)->profData.methodname[i];

		return strcmp(bufx,buf);
}	

int compare_called(MWListItem **firstArg, MWListItem **secondArg)
{
	if ((*firstArg)->profData.called > (*secondArg)->profData.called)
			return 1;
	else if ((*firstArg)->profData.called < (*secondArg)->profData.called)
			return -1;
	else 
		return 0;
}

int rcompare_called(MWListItem **firstArg, MWListItem **secondArg)
{
	if ((*firstArg)->profData.called < (*secondArg)->profData.called)
			return 1;
	else if ((*firstArg)->profData.called > (*secondArg)->profData.called)
			return -1;
	else 
		return 0;
}

int compare_usec(MWListItem **firstArg, MWListItem **secondArg)
{
	if ((*firstArg)->profData.usec > (*secondArg)->profData.usec)
		return 1;
	else if ((*firstArg)->profData.usec < (*secondArg)->profData.usec)
		return -1;
	else 
		return 0;
}

int rcompare_usec(MWListItem **firstArg, MWListItem **secondArg)
{
	if ((*firstArg)->profData.usec < (*secondArg)->profData.usec)
		return 1;
	else if ((*firstArg)->profData.usec > (*secondArg)->profData.usec)
		return -1;
	else 
		return 0;
}

int compare_user(MWListItem **firstArg, MWListItem **secondArg)
{
	if ((*firstArg)->profData.user > (*secondArg)->profData.user)
		return 1;
	else if ((*firstArg)->profData.user < (*secondArg)->profData.user)
		return -1;
	else 
		return 0;
}


int rcompare_user(MWListItem **firstArg, MWListItem **secondArg)
{
	if ((*firstArg)->profData.user < (*secondArg)->profData.user)
		return 1;
	else if ((*firstArg)->profData.user > (*secondArg)->profData.user)
		return -1;
	else 
		return 0;
}

int compare_kernel(MWListItem **firstArg, MWListItem **secondArg)
{
	if ((*firstArg)->profData.kernel > (*secondArg)->profData.kernel)
		return 1;
	else if ((*firstArg)->profData.kernel < (*secondArg)->profData.kernel)
		return -1;
	else 
		return 0;
}

int rcompare_kernel(MWListItem **firstArg, MWListItem **secondArg)
{
	if ((*firstArg)->profData.kernel < (*secondArg)->profData.kernel)
		return 1;
	else if ((*firstArg)->profData.kernel > (*secondArg)->profData.kernel)
		return -1;
	else 
		return 0;
}

int compare_clock(MWListItem **firstArg, MWListItem **secondArg)
{
	if ((*firstArg)->profData.clock > (*secondArg)->profData.clock)
		return 1;
	else if ((*firstArg)->profData.clock < (*secondArg)->profData.clock)
		return -1;
	else 
		return 0;
}

int rcompare_clock(MWListItem **firstArg, MWListItem **secondArg)
{
	if ((*firstArg)->profData.clock < (*secondArg)->profData.clock)
		return 1;
	else if ((*firstArg)->profData.clock > (*secondArg)->profData.clock)
		return -1;
	else 
		return 0;
}