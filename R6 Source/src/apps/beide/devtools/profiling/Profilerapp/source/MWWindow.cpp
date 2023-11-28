#include "MWWindow.h"





const BRect kWindowFrame (100,50,740,400);
const BRect ListFrame (0,70,600,700);
const BRect kButtonFrame (0, 45, 180, 59);
const BRect kButtonFrame2 (179, 45, 255, 59);
const BRect kButtonFrame3 (254, 45, 355, 59);
const BRect kButtonFrame4 (354, 45, 434, 59);
const BRect kButtonFrame5 (433, 45, 516, 59);
const BRect kButtonFrame6 (515, 45, 640, 59);
const BRect kMenuFrame (0,0,600,40);
MWAbout *AboutWindow;


MWWindow::MWWindow() :
	BWindow(kWindowFrame, "New", B_TITLED_WINDOW,NULL)
{
	
	SetSizeLimits(50,640,50,700);
	MainView= new BView(Bounds(), "Main View", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	SetZoomLimits(660,450);
	SetUpMenuBar();
	scrollViewBounds=Bounds();
	scrollViewBounds.PrintToStream();
	scrollViewBounds.right -=15;
	scrollViewBounds.top +=70;

	
	ProfListView=new BListView(scrollViewBounds,"ListView",B_MULTIPLE_SELECTION_LIST,B_FOLLOW_ALL_SIDES);

	ListScrollView= new BScrollView("ListScrollView", ProfListView,B_FOLLOW_ALL_SIDES,B_WILL_DRAW,false,true,B_NO_BORDER);
	AddChild(MainView);

	MainView->AddChild(new BButton(kButtonFrame, "method", "Method Name",	new BMessage(Sort_By_Method)));
	MainView->AddChild(new BButton(kButtonFrame2, "called", "# of Calls",	new BMessage(Sort_By_Call)));
	MainView->AddChild(new BButton(kButtonFrame3, "sec", "µsec/call",	new BMessage(Sort_By_Usec)));
	MainView->AddChild(new BButton(kButtonFrame4, "user", "User",	new BMessage(Sort_By_User)));
	MainView->AddChild(new BButton(kButtonFrame5, "kernel", "Kernel",	new BMessage(Sort_By_Kernel)));
	MainView->AddChild(new BButton(kButtonFrame6, "clock", "WallClock",	new BMessage(Sort_By_Clock)));
	MainView->AddChild(ListScrollView);
	
	MainView->SetViewColor(Black);
	
}

MWWindow::~MWWindow()
{
	/*  Delete all the BListItems from the ListView when the window is destroyed */
	for (int x=0; x < ProfListView->CountItems(); x++)
	{
	
		ProfListView->RemoveItem(x);
		delete ProfListView->ItemAt(x);
	}
	printf("Deleted all items from list view!\n");
}


void
MWWindow::SetUpMenuBar(void)
{
	FileMenu	= new BMenu("File",B_ITEMS_IN_COLUMN);
	EditMenu	= new BMenu("Edit",B_ITEMS_IN_COLUMN);
	SortMenu	= new BMenu("Sort",B_ITEMS_IN_COLUMN);

	MenuBar		= new BMenuBar(kMenuFrame, "MenuBar",B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);

	
	SortMenu->SetRadioMode(true);
	MainView->AddChild(MenuBar);
	MenuBar->AddItem(FileMenu);
	MenuBar->AddItem(EditMenu);
	MenuBar->AddItem(SortMenu);
	
	FileMenu->AddItem(new BMenuItem("Open...", new BMessage(B_OPEN_REQUESTED),*"O",NULL));
	FileMenu->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_WINDOW),*"W",NULL));
	FileMenu->AddItem(new BSeparatorItem);
	FileMenu->AddItem(new BMenuItem("Print", NULL,*"P",NULL));
	MenuBar->FindItem("Print")->SetEnabled(false);
	FileMenu->AddItem(new BMenuItem("About Profiler", new BMessage(B_ABOUT_REQUESTED),*"I",NULL));
	FileMenu->AddItem(new BSeparatorItem);
	FileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_DAMNIT),*"Q",NULL));
	
	EditMenu->AddItem(new BMenuItem("Copy", new BMessage(B_COPY),*"C",NULL));
	
	EditMenu->AddItem(new BMenuItem("Select All", new BMessage(B_SELECT_REQUESTED),*"A",NULL));
	
	SortMenu->AddItem(NameFuncMenuItem=new BMenuItem("Method Name",new BMessage(Sort_By_Method), *"1", NULL));
	SortMenu->AddItem(CallsFunMenuItem=new BMenuItem("Calls",new BMessage(Sort_By_Call), *"2", NULL));
	SortMenu->AddItem(UsecFunMenuItem=new BMenuItem("µsec/call",new BMessage(Sort_By_Usec), *"3", NULL));
	SortMenu->AddItem(UserFunMenuItem=new BMenuItem("User",new BMessage(Sort_By_User), *"4",NULL));
	SortMenu->AddItem(KernelFunMenuItem=new BMenuItem("Kernel",new BMessage(Sort_By_Kernel), *"5", NULL));
	SortMenu->AddItem(WallclockFunMenuItem=new BMenuItem("WallClock",new BMessage(Sort_By_Clock), *"6", NULL));
}

/* Lets read the profile file and add the info to the ListView */
void
MWWindow::ReadFile(char *filename)
{
	short x=0,i=0;
	long double tcalled,tusec,tuser,tkernel,tclock;
	BFont font;
	char buf[360]="\0";
	char buf2[80]="\0";
	ifstream fin(filename);
	fin.getline(buf2,80);
	if (buf2[0]=='W')
	{
		strncat(buf,buf2, strlen(buf2));
		
		
		fin.getline(buf2,80);
		strncat(buf,buf2, strlen(buf2));
		
		
		fin.getline(buf2,80);
		strncat(buf,buf2, strlen(buf2));
		
		
		fin.getline(buf2,80);
		strncat(buf,buf2, strlen(buf2));
	
	(new BAlert("", buf,"OK"))->Go();
	
	fin.getline(buf2,80);

	}
	memset(buf,NULL,360);
	
	/*  Make two BStringViews which show the MaxDepth and the time of the profile */
	depthStringRect.Set(30,25,font.StringWidth(buf2)+50,40);
	depthString = new BStringView(depthStringRect, "depth", buf2);
	depthString->SetFont(be_bold_font);
	depthString->SetDrawingMode(B_OP_OVER);
	depthString->SetHighColor(Green);
	depthString->SetFontSize(11);
	depthString->SetLowColor(B_TRANSPARENT_32_BIT);
	fin.getline(buf,80);

	createdStringRect.Set(depthStringRect.right+218,25,depthStringRect.right+250+font.StringWidth(buf),40);
	createdString = new BStringView(createdStringRect, "created", buf);
	createdString->SetFont(be_bold_font);
	createdString->SetDrawingMode(B_OP_OVER);
	createdString->SetHighColor(Green);
	createdString->SetFontSize(11);

	createdString->SetLowColor(B_TRANSPARENT_32_BIT);
	MainView->AddChild(depthString);
	MainView->AddChild(createdString);
	
	fin.getline(buf,80);
	while (fin.good())
	{
		if ((fin.peek())=='\t')	
			while ((fin.peek())=='\t')
				fin.get();
		
		else if (x==0){
			fin >> tcalled;
			x++;}
		else if (x==1){
			fin >> tusec;
			x++;}	
		else if  (x==2)
		{
			while ((fin.peek())!='\t')
			{
				buf[i]=fin.get();
				i++;
			}
			
			buf[i]=fin.get();
			buf[i++]='\0';
			i=0;		
			x++;
			}
		else if (x==3){
			fin >> tuser;
			x++;}
		else if (x==4){
			fin >> tkernel;
			x++;
			}
		else if (x==5)
		{
			fin >> tclock;
			/* add the stuff to the ListView one line at a time */
			ProfListView->AddItem(new MWListItem(buf,tcalled,tusec,tuser,tkernel,tclock));
			fin.get();
			x=0;
		}
	}
	fin.close();
}

 

bool
MWWindow::QuitRequested()
{
	int i=0;
	i=be_app->CountWindows();
	/* only quit if there was only one window left, otherwise hide the window and destroy it */
	if (i==1)
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
		this->Hide();
		this->Quit();		
	}
	else
	{

	this->Hide();
	this->Quit();

	}
}

void
MWWindow::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		/* Quit the application */
		case B_QUIT_DAMNIT:
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		
		/* User wants to open another profile file */
		case B_OPEN_REQUESTED:
		{
			OpenDialog= new BFilePanel(B_OPEN_PANEL,new BMessenger(be_app, NULL, NULL), NULL,0,false,NULL,NULL,false,true);
			OpenDialog->Show();
			 
			break;
		}
		
		/* User for some reason wants to know about the application and who wrote it */
		case B_ABOUT_REQUESTED:
		{
			
		if (!AboutBox)
		{
			AboutWindow= new MWAbout();
			AboutWindow->Show();
			AboutBox=true;
		}
		else
			AboutWindow->Activate(true);

		break;

		}
		
		/*  User wants to copy some of the rows */
		case B_COPY:
		{
			BClipboard *be_clipboard=new BClipboard("system");
			char *text=NULL, *text2=NULL;
			char *temptext,*temptext2=NULL;
			MWListItem *item=NULL; 
			int32 selected; 
			int total=0,spot=0,range=0;
			int mem=1000;
			bool check=true;
			text=new char[mem+1];
			
			if ( be_clipboard->Lock() ) 
			{ 
       			while ( (selected = ProfListView->CurrentSelection()) >= 0 ) 
   				{
					item=(MWListItem *)ProfListView->ItemAt(selected);
      				total=0;
      				temptext=item->MakeString(&total);
      				range=spot+total;
      				if (range >= mem)
      				{
      					mem=mem+mem+1;
      					temptext2=new char[mem];
      					if (!temptext2)
      						cout<<"could not allocate memory\n";
      					strcpy(temptext2, text);
      					delete []text;
      					text=temptext2;
      					
      				}
      				if (check)
      				{
      					strcpy(text,temptext);
      					delete []temptext;
      					check=false;
      					spot=spot+total;
      				}
      				else{
      					strcpy(&text[spot],temptext);
      					spot=spot+total;
      					delete []temptext;
      				}

      				ProfListView->Deselect(selected);
      				ProfListView->Flush();
      			}
      		
			cout <<"Putting text into Clipboard\n";
			be_clipboard->Clear(); 
   			BMessage *clipper = be_clipboard->Data(); 
   			clipper->what=B_ANY_TYPE;
			clipper->AddData("text/plain", B_MIME_TYPE, text, strlen(text));
  			be_clipboard->Commit(); 	
			be_clipboard->Unlock(); 
			delete []text;
			}
		break;
		}
		
		/* User wants to close a window.  Quit if it was the last window, otherwise quit the app */
		case B_CLOSE_WINDOW:
		{

			this->Hide();
			
			if (be_app->CountWindows()==1)
				be_app->PostMessage(B_QUIT_REQUESTED);
			this->Quit();
			break;
		}
		
		/* User hits cancel on the open dialog, so lets get rid of the Open Dialog box */
		case B_CANCEL :
		{
			delete OpenDialog;
			break;
		}
		
		/* User clicks on one of the ListItems, so lets select it */
		case B_SELECT_REQUESTED:
		{
			ProfListView->Select(0, ProfListView->CountItems()-1,false);
			MenuBar->FindItem("Copy")->SetEnabled(true);
			
			break;
		}
		
		/* This sorts the ListView by the Method names in asending or descending*/
		case Sort_By_Method:
		{
			
			if (!bname)
			{
			
				ProfListView->SortItems((int (*)(const void*, const void*))compare_name);
				bname=1;
				bcalled=busec=buser=bkernel=bclock=0;
				NameFuncMenuItem->SetMarked(true);
			}
			else
			{
				ProfListView->SortItems((int (*)(const void*, const void*))rcompare_name);
				bname=0;
			}
			break;
		}
		
		/* This sorts the ListView by the amount of times the method was called names in 
		   asending or descending*/
		case Sort_By_Call:
		{
			
			if (!bcalled)
			{
				ProfListView->SortItems((int (*)(const void*, const void*))compare_called);
				bcalled=1;
				bname=busec=buser=bkernel=bclock=0;
				CallsFunMenuItem->SetMarked(true);
			}
			else
			{
				ProfListView->SortItems((int (*)(const void*, const void*))rcompare_called);
				bcalled=0;
			}
			break;
		}
		case Sort_By_Usec:
		{
		
			if(!busec)
			{
				ProfListView->SortItems((int (*)(const void*, const void*))compare_usec);
				busec=1;
				bcalled=bname=buser=bkernel=bclock=0;
				UsecFunMenuItem->SetMarked(true);
			}
			else
			{
				ProfListView->SortItems((int (*)(const void*, const void*))rcompare_usec);
				busec=0;
			}
			break;
		}
		
	
		case Sort_By_User:
		{
			if(!buser)
			{
				ProfListView->SortItems((int (*)(const void*, const void*))compare_user);
				buser=1;
				bcalled=busec=bname=bkernel=bclock=0;
				UserFunMenuItem->SetMarked(true);
			}
			else{
				ProfListView->SortItems((int (*)(const void*, const void*))rcompare_user);
				buser=0;
				}
			break;
		}
	
		case Sort_By_Kernel:
		{
			
			if(!bkernel)
			{
				ProfListView->SortItems((int (*)(const void*, const void*))compare_kernel);
				bkernel=1;
				bcalled=busec=bname=buser=bclock=0;
				KernelFunMenuItem->SetMarked(true);
			}
			else{
				ProfListView->SortItems((int (*)(const void*, const void*))rcompare_kernel);
				bkernel=0;
				}
			break;
		}
	
		case Sort_By_Clock:
		{
			if (!bclock)
			{
				ProfListView->SortItems((int (*)(const void*, const void*))compare_clock);
				bclock=1;
				bcalled=busec=bname=buser=bkernel=0;
				WallclockFunMenuItem->SetMarked(true);
			}
			else{
				ProfListView->SortItems((int (*)(const void*, const void*))rcompare_clock);
				bclock=0;
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}
