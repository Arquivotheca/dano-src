#include <stdio.h>
#include <Resources.h>

#include <private/prefpanel/PrefsAppExport.h>

BMessage* ReadMessageFromResource(BResources* r,const char* name) {
	size_t res_size;
	const void* res_addr;
	BMessage* msg=new BMessage;
	if (((res_addr=r->LoadResource(B_MESSAGE_TYPE,name,&res_size))==NULL)
	 ||(msg->Unflatten((const char*)res_addr)!=B_OK)) {
		delete msg;
		return NULL;
	}
	return msg;
}

BMessage* ReadMessageFromResource(BResources* r,int32 id) {
	size_t res_size;
	const void* res_addr;
	BMessage* msg=new BMessage;
	if (((res_addr=r->LoadResource(B_MESSAGE_TYPE,id,&res_size))==NULL)
	 ||(msg->Unflatten((const char*)res_addr)!=B_OK)) {
		delete msg;
		return NULL;
	}
	return msg;
}

void PrintMessageToStream(BMessage* m,uint32 reclevel) {
	m->PrintToStream();
	BMessage msg;
	for (int nmess=0;m->FindMessage("_msg",nmess,&msg)==B_OK;nmess++) {
		printf("recursing _msg [%ld]\n",reclevel+1);
		PrintMessageToStream(&msg,reclevel+1);
		printf("end _msg recursion [%ld]\n",reclevel+1);
	}
	for (int nmess=0;m->FindMessage("_views",nmess,&msg)==B_OK;nmess++) {
		printf("recursing _views [%ld]\n",reclevel+1);
		PrintMessageToStream(&msg,reclevel+1);
		printf("end _views recursion [%ld]\n",reclevel+1);
	}
	for (int nmess=0;m->FindMessage("_items",nmess,&msg)==B_OK;nmess++) {
		printf("recursing _items [%ld]\n",reclevel+1);
		PrintMessageToStream(&msg,reclevel+1);
		printf("end _items recursion [%ld]\n",reclevel+1);
	}
	for (int nmess=0;m->FindMessage("_submenu",nmess,&msg)==B_OK;nmess++) {
		printf("recursing _submenu [%ld]\n",reclevel+1);
		PrintMessageToStream(&msg,reclevel+1);
		printf("end _submenu recursion [%ld]\n",reclevel+1);
	}
}

BArchivable* InstantiateFromResource(BResources* res,int32 id) {
	BMessage* m=ReadMessageFromResource(res,id);
	if (m!=NULL) {
		BArchivable* arc=instantiate_object(m);	
		delete m;
		if (arc==NULL) {
			printf("could not instantiate object\n");
		}
		return arc;
	} else {
		printf("ReadMessageFromResource returned NULL\n");
		return NULL;
	}
}

BArchivable* InstantiateFromResource(BResources* res,const char* name) {
	BMessage* m=ReadMessageFromResource(res,name);
	if (m!=NULL) {
		BArchivable* arc=instantiate_object(m);	
		delete m;
		if (arc==NULL) {
			printf("could not instantiate object\n");
		}
		return arc;
	} else {
		printf("ReadMessageFromResource returned NULL\n");
		return NULL;
	}
}

BarView::BarView(BMessage*m):BView(m) {
	if (m->FindInt32("orientation",&orientation)!=B_OK) {
		orientation=HORIZONTAL_BAR_VIEW;
	}
}

void BarView::AttachedToWindow() {
	if (orientation==HORIZONTAL_BAR_VIEW) {
		ResizeTo(Bounds().Width(),1);
	} else {
		ResizeTo(1,Bounds().Height());
	}
}

void BarView::Draw(BRect) {
	if (orientation==HORIZONTAL_BAR_VIEW) {
		StrokeLine(BPoint(0,1),Bounds().RightBottom());
	} else {
		StrokeLine(BPoint(1,0),Bounds().RightBottom());
	}
}

BArchivable* BarView::Instantiate(BMessage*m) {
	if (!validate_instantiation(m,"BarView")) {
		return NULL;
	}
	return new BarView(m);                   
}
