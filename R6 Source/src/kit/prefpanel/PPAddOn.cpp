#include <private/prefpanel/PPAddOn.h>
#include <private/prefpanel/PPWindow.h>
#include <Entry.h>
#include <Resources.h>
#include <stdio.h>

PPAddOn::PPAddOn(image_id i,PPWindow*w) {
	MyIid=i;
	Window=w;
	NextAddOn=NULL;
	Resources=NULL;
	InitResources();
}

PPAddOn::~PPAddOn() {
	delete Resources; // it is legal to delete NULL;
}

void PPAddOn::InitResources() {
	printf("PPAddOn::InitResources\n");
	Resources=new BResources;
	image_info iinf;
	get_image_info(MyIid,&iinf);
	entry_ref ref(iinf.device,iinf.node,iinf.name);
	BFile f(&ref,B_READ_ONLY);
	if (f.InitCheck()!=B_OK) {
		return;
	}
	if (Resources->SetTo(&f)!=B_OK) {
		printf("PPAddOn::InitResources : SetTo error\n");
	}
}

bool PPAddOn::UseAddOn() {
	return true;
}

BBitmap* PPAddOn::SmallIcon() {
	return new BBitmap(Icon());
}

void PPAddOn::LoadPrefs(BMessage*) {}

void PPAddOn::SavePrefs(BMessage*) {}

bool PPAddOn::QuitRequested() {
	return true;
}

void PPAddOn::PanelActivated(bool) {}

void PPAddOn::ChangeIcon(BBitmap*large,BBitmap*small) {
	Window->ChangeIcon(this,large,small);
}

void PPAddOn::UseNewPanel(PPAddOn*a) {
	Window->UseNewPanel(a);
}

void PPAddOn::RemovePanel(PPAddOn*a) {
	Window->RemovePanel(a);
}

void PPAddOn::SwitchToPanel(const char*n) {
	Window->SwitchToPanel(n ? n : InternalName());
}
