#include <Application.h>
#include <Window.h>

#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>
#include <View.h>
#include <Alert.h>
#include <Bitmap.h>
#include <StringView.h>
#include <SlideRule.h>

#include <fs_attr.h>

#include <stdio.h>
#include <image.h>
#include <stdlib.h>
#include <string.h>

#include <private/prefpanel/PPAddOn.h>
#include <private/prefpanel/PrefsAppExport.h>
#include <private/prefpanel/PPWindow.h>
#include "ListAddOns.h"

#include <Resources.h>
#include <AppFileInfo.h>

/////////////////////////////////

#define ALERT_TITLE "Fatal Error"
#define ALERT_TEXT "Fatal error - The preferences panel cannot run\n\n[%s - %d] (%s)"
#define ALERT_BUTTON "Too bad!"

/////////////////////////////////

BResources* app_resources;

void ErrorAlert(char* filename,int lineno,char* reason,int32 resource_id);

/////////////////////////////////

class PPApplication : public BApplication {
public:
	PPApplication(const char*);
	void ArgvReceived(int32 argc,char** argv);
	void ReadyToRun();
	void MessageReceived(BMessage*);

	bool is_running;
	char* launch_name;
	PPWindow* the_window;
};

/////////////////////////////////

PPApplication::PPApplication(const char*s):BApplication(s) {
	is_running=false;
	launch_name=NULL;
	the_window=(PPWindow*)InstantiateFromResource(app_resources,"main window");
}

void PPApplication::ArgvReceived(int32 argc,char** argv) {
	printf("PPApplication::ArgvReceived\n");
	if (argc==1) {
		return;
	}
	if (!is_running) {
		printf("will need to launch %s\n",argv[1]);
		launch_name=strdup(argv[1]);
	} else {
		printf("already running - will switch to %s\n",argv[1]);
		the_window->SwitchToPanel(argv[1]);
	}
}

void PPApplication::ReadyToRun() {
	is_running=true;
//	the_window=(PPWindow*)InstantiateFromResource(app_resources,"main window");
	the_window->LoadAddOns(launch_name);
	if (the_window->FirstAddOn==NULL) {
		ErrorAlert(__FILE__,__LINE__,"No add-ons",1020);
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	the_window->Show();
}

void PPApplication::MessageReceived(BMessage*m) {
	if (m->HasString("be:panel")) {
		printf("app : magic message\n");
		the_window->PostMessage(m);
	} else {
		BApplication::MessageReceived(m);
	}
}

///////////////////////////////////

PPWindow::PPWindow(BMessage* m):BWindow(m) {
	preferences=new BMessage;
	FirstAddOn=NULL;
	CurrentAddOn=NULL;
	BFont f(be_bold_font);
	f.SetSize(18);
	((BStringView*)FindView("addontitle"))->SetFont(&f);
}

#define ICONSPACING 56
#define SIZEFUDGE 12

#define min(a, b) ((a)<(b)?(a):(b))

void PPWindow::LoadAddOns(const char* launchname) {
	BView *darkgrey = new BView(BRect(0, 0, 109, Frame().Height()), "gray", B_FOLLOW_LEFT|B_FOLLOW_TOP_BOTTOM, 0);
	darkgrey->SetViewColor(156, 156, 156);
	FindView("backview")->AddChild(darkgrey);
	BSlideRule *sr = new BSlideRule(BRect(5,5,104,Frame().Height() - 5),"sliderule",
				new BMessage('slct'),B_SR_RIGHT_ARROW,B_FOLLOW_LEFT|B_FOLLOW_TOP,B_NAVIGABLE);
	sr->SetArrowPosition(0);
	sr->SetArrowMargin(30);
	sr->SetArrowSize(8);
	sr->SetIconSpacing(ICONSPACING);
	sr->SetValue(0);
	sr->SetWrap(true);
	darkgrey->AddChild(sr);

	BPath path;
	size_t foo_size;
	find_directory(B_USER_SETTINGS_DIRECTORY,&path,true);
	char* fname=(char*)app_resources->LoadResource(B_STRING_TYPE,"settings file",&foo_size);
	path.Append(fname);
	BFile file(path.Path(),B_READ_ONLY);
	if (file.InitCheck()==B_OK) {
		preferences->Unflatten(&file);
	}
	ReadPrefsFromAttributes(preferences,&file);

	PPAddOnData**current=&FirstAddOn;
	size_t addonpathsize;
	char* addonpath=(char*)app_resources->LoadResource(B_STRING_TYPE,100,&addonpathsize);
	char* addons=ListAddOns(addonpath);
	if (addons!=NULL) {
		char* tok=addons;
		char* ttok;
		while ((tok=strtok_r(tok,":",&ttok))!=NULL) {
			PPAddOn*this_add_on=LoadAnAddOn(tok);
			tok=NULL;
			if (this_add_on==NULL) {
				continue;
			}
			*current=new PPAddOnData;
			(*current)->AddOn=this_add_on;
			(*current)->next=NULL;
			InitAnAddOn(*current);
			while (*current!=NULL) {
				current=&((*current)->next);
			}
		}
		delete addons;
	}

	BMessage panelprefs;
	preferences->FindMessage("panel",&panelprefs);

	CurrentAddOn=AddOnDataForName(launchname);
	if (CurrentAddOn==NULL) {
		CurrentAddOn=AddOnDataForName(panelprefs.FindString("currentpanel"));
	}
	if (CurrentAddOn==NULL && app_resources!=NULL) {
		size_t foo_size;
		const char *DefaultAddOn = (const char*)app_resources->LoadResource(B_STRING_TYPE,"default add-on",&foo_size);
		CurrentAddOn=AddOnDataForName(DefaultAddOn);
	}
	if (CurrentAddOn==NULL) {
		CurrentAddOn=FirstAddOn;
	}

	sr->MakeFocus();
	sr->SetValue(IntForAddOn(CurrentAddOn->AddOn));
	ResizeTo(CurrentAddOn->View->Bounds().Width()+109+21,CurrentAddOn->View->Bounds().Height()+61);
	sr->ResizeTo(sr->Bounds().Width(), min(sr->CountIcons() * ICONSPACING + SIZEFUDGE, Bounds().Height() - 5));
	FindView("addonview")->ResizeTo(CurrentAddOn->View->Bounds().Width(),CurrentAddOn->View->Bounds().Height());
	((BStringView*)FindView("addontitle"))->SetText(CurrentAddOn->Name);
	CurrentAddOn->AddOn->PanelActivated(true);
	CurrentAddOn->View->Show();

	if (panelprefs.HasPoint("windowlocation")) {
		MoveTo(panelprefs.FindPoint("windowlocation"));
		panelprefs.FindPoint("windowlocation").PrintToStream();
	}

#if 0
	printf("found %d add-ons\n",NumAddOns);

	if (NumAddOns==0) {
		printf("no add-ons found\n");
		return;
	}
#endif
/*
	AddOns=new PPAddOn*[NumAddOns];
	Icons=new BBitmap*[NumAddOns];
	SmallIcons=new BBitmap*[NumAddOns];
	Names=new char*[NumAddOns];
	InternalNames=new char*[NumAddOns];
	Views=new BView*[NumAddOns];

	printf("allocated storage for the add-on data\n");

#if 0
	FindView("backview")->AddChild(new BSlideRule(BRect(0,0,99,Frame().Height()),"sliderule",
				new BMessage('slct'),B_SR_RIGHT_ARROW,B_FOLLOW_LEFT|B_FOLLOW_TOP_BOTTOM,B_NAVIGABLE));
	((BSlideRule*)FindView("sliderule"))->SetArrowPosition(.125);
	((BSlideRule*)FindView("sliderule"))->SetValue(0);

	BPath path;
	size_t foo_size;
	find_directory(B_USER_SETTINGS_DIRECTORY,&path,true);
	char* fname=(char*)app_resources->LoadResource(B_STRING_TYPE,"settings file",&foo_size);
	path.Append(fname);
	BFile file(path.Path(),B_READ_ONLY);
	if (file.InitCheck()==B_OK) {
		preferences->Unflatten(&file);
	}
#endif

	BMessage panelprefs;
	preferences->FindMessage("panel",&panelprefs);
	const char*usename=panelprefs.FindString("currentpanel");

	printf("about to do the add-ons loading loop\n");
	for (int i=0;i<NumAddOns;i++) {
		AddOns[i]=add_on[i];
		printf("will get the internal name for add-on #%d\n",i);
		InternalNames[i]=AddOns[i]->InternalName();
		printf("got the internal name\n");
		BMessage localprefs;
		preferences->FindMessage(InternalNames[i],&localprefs);
		int32 numitems=localprefs.CountNames(B_ANY_TYPE);
		type_code typefound;
		char* namefound;
		int32 countfound;
		printf("will walk the fields in the preferences\n");
		for (int32 j=0;j<numitems;j++) {
			localprefs.GetInfo(B_ANY_TYPE,j,&namefound,&typefound,&countfound);
			if ((countfound==1)&&(namefound[0]!='$')) {
				printf("found field %s\n",namefound);
				char fullname[B_OS_NAME_LENGTH];
				sprintf(fullname,"%s:%s",InternalNames[i],namefound);
				printf("full name %s\n",fullname);
				attr_info ai;
				if ((file.GetAttrInfo(fullname,&ai)==B_OK)&&(ai.type==typefound)) {
					switch(typefound) {
						case B_INT8_TYPE : {
							int8 v;
							if (file.ReadAttr(fullname,typefound,0,&v,sizeof(v))==sizeof(v)) {
								printf("replacing %s with %d\n",namefound,v);
								localprefs.ReplaceInt8(namefound,v);
							}
							break;
						}
						case B_INT16_TYPE : {
							int16 v;
							if (file.ReadAttr(fullname,typefound,0,&v,sizeof(v))==sizeof(v)) {
								printf("replacing %s with %d\n",namefound,v);
								localprefs.ReplaceInt16(namefound,v);
							}
							break;
						}
						case B_INT32_TYPE : {
							int32 v;
							if (file.ReadAttr(fullname,typefound,0,&v,sizeof(v))==sizeof(v)) {
								printf("replacing %s with %ld\n",namefound,v);
								localprefs.ReplaceInt32(namefound,v);
							}
							break;
						}
						case B_INT64_TYPE : {
							int64 v;
							if (file.ReadAttr(fullname,typefound,0,&v,sizeof(v))==sizeof(v)) {
								printf("replacing %s with %Ld\n",namefound,v);
								localprefs.ReplaceInt64(namefound,v);
							}
							break;
						}
						case B_FLOAT_TYPE : {
							float v;
							if (file.ReadAttr(fullname,typefound,0,&v,sizeof(v))==sizeof(v)) {
								printf("replacing %s with %f\n",namefound,v);
								localprefs.ReplaceFloat(namefound,v);
							}
							break;
						}
						case B_DOUBLE_TYPE : {
							double v;
							if (file.ReadAttr(fullname,typefound,0,&v,sizeof(v))==sizeof(v)) {
								printf("replacing %s with %f\n",namefound,v);
								localprefs.ReplaceDouble(namefound,v);
							}
							break;
						}
						case B_BOOL_TYPE : {
							bool v;
							if (file.ReadAttr(fullname,typefound,0,&v,sizeof(v))==sizeof(v)) {
								localprefs.ReplaceBool(namefound,v);
							}
							break;
						}
						case B_STRING_TYPE : {
							printf("it's a string\n");
							char* v=new char[ai.size];
							if (file.ReadAttr(fullname,typefound,0,v,ai.size)==ai.size) {
								localprefs.ReplaceString(namefound,v);
							}
							delete[] v;
							break;
						}
						default : {
							break;
						}
					}
				}
			}
		}
		printf("walked the fields in the preferences\n");

		AddOns[i]->LoadPrefs(&localprefs);
		Icons[i]=AddOns[i]->Icon();
		SmallIcons[i]=AddOns[i]->SmallIcon();
		Names[i]=AddOns[i]->Name();
		Views[i]=AddOns[i]->MakeView();
		Views[i]->MoveBy(32,32);
		FindView("addonview")->AddChild(Views[i]);
//		Views[i]->AddChild(new BStringView(BRect(0,0,199,14),"panel title",Names[i]));
		Views[i]->Hide();

		printf("will add icon in the SlideRule\n");

		((BSlideRule*)FindView("sliderule"))->AddIcon(Icons[i],SmallIcons[i]);
		if (usename!=NULL&&strcmp(usename,InternalNames[i])==0) {
			current_add_on=i;
		}
	}
	printf("add-on loading loop done\n");

	if (panelprefs.HasPoint("windowlocation")) {
		MoveTo(panelprefs.FindPoint("windowlocation"));
		panelprefs.FindPoint("windowlocation").PrintToStream();
	}

	FindView("sliderule")->MakeFocus();
	((BSlideRule*)FindView("sliderule"))->SetValue(current_add_on);
	ResizeTo(Views[current_add_on]->Bounds().Width()+100+32,Views[current_add_on]->Bounds().Height()+32);
	FindView("addonview")->ResizeTo(Views[current_add_on]->Bounds().Width()+32,Views[current_add_on]->Bounds().Height()+32);
	((BStringView*)FindView("addontitle"))->SetText(Names[current_add_on]);
	AddOns[current_add_on]->PanelActivated(true);
	Views[current_add_on]->Show();
*/
}

PPAddOn*PPWindow::LoadAnAddOn(const char*mtok) {
	image_id i=load_add_on(mtok);
	if (i<B_OK) {
		printf("error loading add-on\n");
		return NULL;
	}
	PPAddOn*(*func)(image_id,PPWindow*);
	printf("trying to find entry function\n");
	if (get_image_symbol(i,"get_addon_object",B_SYMBOL_TYPE_TEXT,(void**)&func)!=B_OK) {
		printf("error finding function\n");
		unload_add_on(i);
		return NULL;
	}
	PPAddOn* this_add_on=func(i,this);
	if (!this_add_on->UseAddOn()) {
		delete this_add_on;
		unload_add_on(i);
		return NULL;
	}
	return this_add_on;
}

void PPWindow::InitAnAddOn(PPAddOnData*aod) {
	BMessage localprefs;

	aod->InternalName=aod->AddOn->InternalName();

	preferences->FindMessage(aod->InternalName,&localprefs);

	aod->AddOn->LoadPrefs(&localprefs);

	aod->Icon=aod->AddOn->Icon();
	aod->SmallIcon=aod->AddOn->SmallIcon();
	aod->Name=aod->AddOn->Name();
	((BSlideRule*)FindView("sliderule"))->AddIcon(aod->Name,aod->Icon,aod->SmallIcon);

	aod->View=aod->AddOn->MakeView();

//	aod->View->MoveBy(32,32);
	FindView("addonview")->AddChild(aod->View);
	aod->View->Hide();
}

void PPWindow::MessageReceived(BMessage*m) {
	if (m->what=='slct') {
		int32 numicon=m->FindInt32("be:value");
		printf("PPWindow::MessageReceived : selected panel #%ld\n",numicon);
		PPAddOnData*np=AddOnDataForInt(numicon);
		if (np==CurrentAddOn) {
			printf("Same add-on\n");
			return;
		}
		SwitchToPanel(np);
	} else {
		if (m->HasString("be:panel")) {
			printf("window : magic message\n");
			PPAddOnData*ao=AddOnDataForName(m->FindString("be:panel"));
			if (ao!=NULL) {
				ao->View->MessageReceived(m);
			}
		} else {
			BWindow::MessageReceived(m);
		}
	}
}

PPAddOnData*PPWindow::AddOnDataForInt(int n) {
	PPAddOnData*r=FirstAddOn;
	for (int i=0;i<n;i++) {
		r=r->next;
	}
	return r;
}

PPAddOnData*PPWindow::AddOnDataForName(const char*n) {
	if (n==NULL) {
		return NULL;
	}
	for (PPAddOnData*current=FirstAddOn;current!=NULL;current=current->next) {
		if (strcmp(n,current->InternalName)==0) {
			return current;
		}
	}
	return NULL;
}

int PPWindow::IntForAddOn(PPAddOn*a) {
	int r=0;
	for (PPAddOnData*current=FirstAddOn;current!=NULL;current=current->next) {
		if (current->AddOn==a) {
			return r;
		}
		r++;
	}
	return -1;
}

void PPWindow::ReadPrefsFromAttributes(BMessage*,BNode*) {
	printf("PPWindow::ReadPrefsFromAttributes : not implemented\n");
}

void PPWindow::WritePrefsToAttributes(BMessage*,BNode*) {
	printf("PPWindow::WritePrefsToAttributes : not implemented\n");
#if 0
		int32 numitems=localprefs.CountNames(B_ANY_TYPE);
		type_code typefound;
		char* namefound;
		int32 countfound;
		for (int32 j=0;j<numitems;j++) {
			localprefs.GetInfo(B_ANY_TYPE,j,&namefound,&typefound,&countfound);
			if ((countfound==1)&&(namefound[0]!='$')) {
				printf("found field %s\n",namefound);
				char fullname[B_OS_NAME_LENGTH];
				sprintf(fullname,"%s:%s",intname,namefound);
				printf("full name %s\n",fullname);
				switch(typefound) {
					case B_INT8_TYPE : {
						int8 v=localprefs.FindInt8(namefound);
						file.WriteAttr(fullname,typefound,0,&v,sizeof(v));
						break;
					}
					case B_INT16_TYPE : {
						int16 v=localprefs.FindInt16(namefound);
						file.WriteAttr(fullname,typefound,0,&v,sizeof(v));
						break;
					}
					case B_INT32_TYPE : {
						int32 v=localprefs.FindInt32(namefound);
						file.WriteAttr(fullname,typefound,0,&v,sizeof(v));
						break;
					}
					case B_INT64_TYPE : {
						int64 v=localprefs.FindInt64(namefound);
						file.WriteAttr(fullname,typefound,0,&v,sizeof(v));
						break;
					}
					case B_FLOAT_TYPE : {
						float v=localprefs.FindFloat(namefound);
						file.WriteAttr(fullname,typefound,0,&v,sizeof(v));
						break;
					}
					case B_DOUBLE_TYPE : {
						double v=localprefs.FindDouble(namefound);
						file.WriteAttr(fullname,typefound,0,&v,sizeof(v));
						break;
					}
					case B_BOOL_TYPE : {
						bool v=localprefs.FindBool(namefound);
						file.WriteAttr(fullname,typefound,0,&v,sizeof(v));
						break;
					}
					case B_STRING_TYPE : {
						file.WriteAttr(fullname,typefound,0,localprefs.FindString(namefound),strlen(localprefs.FindString(namefound))+1);
						break;
					}
					default : {
						break;
					}
				}
			}
		}

#endif
}

bool PPWindow::QuitRequested() {
	for (PPAddOnData*current=FirstAddOn;current!=NULL;current=current->next) {
		if (!current->AddOn->QuitRequested()) {
			return false;
		}
	}

	BPath path;
	size_t foo_size;
	find_directory(B_USER_SETTINGS_DIRECTORY,&path,true);
	char* fname=(char*)app_resources->LoadResource(B_STRING_TYPE,"settings file",&foo_size);
	BFile file;
	BDirectory dir(path.Path());
	BEntry(&dir,fname).Remove();
	dir.CreateFile(fname,&file);

	for (PPAddOnData*current=FirstAddOn;current!=NULL;current=current->next) {
		char* intname=current->InternalName;
		BMessage localprefs;
		printf("save prefs for %s\n",intname);
		current->AddOn->SavePrefs(&localprefs);
		preferences->RemoveName(intname);
		preferences->AddMessage(intname,&localprefs);

	}

	BMessage panelprefs;
	if (FirstAddOn!=NULL) {
		panelprefs.AddString("currentpanel",CurrentAddOn->InternalName);
	}
	panelprefs.AddPoint("windowlocation",Frame().LeftTop());

	printf("will remove global panel preferences, pointer is %p\n",preferences);
	preferences->RemoveName("panel");
	printf("global panel preferences removed\n");
	preferences->AddMessage("panel",&panelprefs);
	preferences->Flatten(&file);
	WritePrefsToAttributes(preferences,&file);

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

BArchivable* PPWindow::Instantiate(BMessage* m) {
	if (!validate_instantiation(m,"PPWindow")) {
		return NULL;
	}
	return new PPWindow(m);
}

void PPWindow::WorkspaceActivated(int32,bool) {
	for (PPAddOnData*current=FirstAddOn;current!=NULL;current=current->next) {
		current->View->MessageReceived(CurrentMessage());
	}
}

void PPWindow::ScreenChanged(BRect,color_space) {
	for (PPAddOnData*current=FirstAddOn;current!=NULL;current=current->next) {
		current->View->MessageReceived(CurrentMessage());
	}
}

void PPWindow::ChangeIcon(PPAddOn*ao,BBitmap*,BBitmap*) {
	if (!IsLocked()) {
		debugger("PPWindow::ChangeIcon : the window must be locked\n");
	}
	printf("request to change icon in addon #%d\n",IntForAddOn(ao));
}

void PPWindow::UseNewPanel(PPAddOn*ao) {
	if (!IsLocked()) {
		debugger("PPWindow::UseNewPanel : the window must be locked\n");
	}
	printf("must use new panel %p\n",ao);
	PPAddOnData**current=&FirstAddOn;
	while ((*current)!=NULL) {
		current=&(*current)->next;
	}
	*current=new PPAddOnData;
	(*current)->AddOn=ao;
	(*current)->next=NULL;
	InitAnAddOn(*current);
}

void PPWindow::RemovePanel(PPAddOn*ao) {
	if (!IsLocked()) {
		debugger("PPWindow::UseNewPanel : the window must be locked\n");
	}
	printf("must remove panel %p\n",ao);

	BeginViewTransaction();

	PPAddOnData*NewAddOn,*ToRemove;
	BSlideRule *sr = (BSlideRule*)FindView("sliderule");
	sr->RemoveIconAt(IntForAddOn(ao));

	if (ao==FirstAddOn->AddOn) {
		ToRemove=FirstAddOn;
		FirstAddOn=FirstAddOn->next;
		NewAddOn=FirstAddOn;
	} else {
		PPAddOnData*current=FirstAddOn;
		while (current->next->AddOn!=ao) {
			current=current->next;
		}
		ToRemove=current->next;
		current->next=current->next->next;
		NewAddOn=current;
	}
	ToRemove->View->RemoveSelf();
	delete ToRemove->View;
	delete ToRemove->AddOn;
	delete ToRemove;

	CurrentAddOn=NewAddOn;
	sr->SetValue(IntForAddOn(CurrentAddOn->AddOn));
	sr->ResizeTo(sr->Bounds().Width(), min(sr->CountIcons() * ICONSPACING + SIZEFUDGE, Bounds().Height() - 5));
	ResizeTo(CurrentAddOn->View->Bounds().Width()+109+21,CurrentAddOn->View->Bounds().Height()+61);
	FindView("addonview")->ResizeTo(CurrentAddOn->View->Bounds().Width(),CurrentAddOn->View->Bounds().Height());
	((BStringView*)FindView("addontitle"))->SetText(CurrentAddOn->Name);
	CurrentAddOn->AddOn->PanelActivated(true);
	CurrentAddOn->View->Show();
	EndViewTransaction();
}

void PPWindow::SwitchToPanel(const char*n) {
	PPAddOnData*dest=AddOnDataForName(n);
	if (dest!=NULL) {
		SwitchToPanel(dest);
	}
}

void PPWindow::SwitchToPanel(PPAddOnData*newpanel) {
	BeginViewTransaction();
	CurrentAddOn->View->Hide();
	CurrentAddOn->AddOn->PanelActivated(false);
	CurrentAddOn=newpanel;
	BSlideRule *sr = ((BSlideRule*)FindView("sliderule"));
	ResizeTo(CurrentAddOn->View->Bounds().Width()+109+21,CurrentAddOn->View->Bounds().Height()+61);
	sr->ResizeTo(sr->Bounds().Width(), min(sr->CountIcons() * ICONSPACING + SIZEFUDGE, Bounds().Height() - 5));
	FindView("addonview")->ResizeTo(CurrentAddOn->View->Bounds().Width(),CurrentAddOn->View->Bounds().Height());
	((BStringView*)FindView("addontitle"))->SetText(CurrentAddOn->Name);
	CurrentAddOn->AddOn->PanelActivated(true);
	CurrentAddOn->View->Show();
	sr->SetValue(IntForAddOn(CurrentAddOn->AddOn));
	EndViewTransaction();
}

////////////////////////////////////////////////////////
// main
////////////////////////////////////////////////////////
// initializes the resources and creates the app.
int main() {
	app_resources=BApplication::AppResources();
	if (app_resources!=NULL) {
		(new PPApplication("application/x-vnd.Be.preferences.generic"))->Run();
		delete be_app;
		return 0;
	}
	BApplication a("application/x-vnd.Be.preferences.generic");
	ErrorAlert(__FILE__,__LINE__,"No resources",1000);
}

/////////////////////////////////////////////////////////
// ErrorAlert - displays an error alert box
/////////////////////////////////////////////////////////
// will try to load alert text from the resources,
// and display it. if it cannot load the text from
// the resources, if will display a standard text,
// along with a caller-supplied reason, filename and
// line number
//
void ErrorAlert(char* filename,int lineno,char* reason,int32 resource_id) {
	if (app_resources!=NULL) {
		size_t foo_size;
		const char*tit=(const char*)app_resources->LoadResource(B_STRING_TYPE,resource_id,&foo_size);
		const char*tex=(const char*)app_resources->LoadResource(B_STRING_TYPE,resource_id+1,&foo_size);
		const char*but=(const char*)app_resources->LoadResource(B_STRING_TYPE,resource_id+2,&foo_size);
		if ((tit!=NULL)&&(tex!=NULL)&&(but!=NULL)) {
			(new BAlert(tit,tex,but,NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
			return;
		}
	}
	char* txt=new char[strlen(ALERT_TEXT)+100];
	sprintf(txt,ALERT_TEXT,filename,lineno,reason);
	(new BAlert(ALERT_TITLE,txt,ALERT_BUTTON,NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
	delete txt;
}

