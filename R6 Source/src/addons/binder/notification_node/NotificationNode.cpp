/*
	NotificationNode.cpp
*/
#include <String.h>

#include <stdlib.h>

#include "NotificationNode.h"

NotificationNode::NotificationNode()
	:	BinderNode()
{

}


NotificationNode::~NotificationNode()
{
	Clear();
}

put_status_t NotificationNode::WriteProperty(const char *, const property &)
{
	return B_ERROR;
}

get_status_t NotificationNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (strcmp("Post", name) == 0) {
		Post(prop, args);
	} else if (strcmp("Next", name) == 0) {
		Next(prop, args);
	} else if (strcmp("Peek", name) == 0) {
		Peek(prop, args);
	} else if (strcmp("Count", name) == 0) {
		Count(prop, args);
	} else if (strcmp("Clear", name) == 0) {
		Clear();
		prop.Undefine();
	} else if (strcmp("PrintToStream", name) == 0) {
		PrintToStream();
		prop.Undefine();
	} else {
		// Unknown action
		prop.Undefine();
		return B_ERROR;
	}
	return B_OK;
}

status_t NotificationNode::Post(property &outProperty, const property_list &inArgs)
{
	// At the very minimum, we need one arg to represent the alert code...
	if (inArgs.Count() < 1) {
		return B_ERROR;
	}
	
	int32 count = inArgs.CountItems();
	NotificationContainer *container = new NotificationContainer();
	for (int32 i = 0; i < count; i++) {
		property *property = inArgs.ItemAt(i);
		switch (i) {
			case 0:		// Alert code
				container->AddProperty("code", *property);
				break;
			case 1:		// Alert priority
				container->AddProperty("priority", *property);
				break;
			default:	// Variable number of parameters..
				container->ReadProperty("AddParameter", *property);
				break;
		}
	}
	// If this alert is not of normal priority, then slot it into
	// the appropriate spot in the alert list...
	if (container->Priority() == kNormalPriority) {
		fAlerts.AddItem(container);
	} else {
		bool inserted = false;
		NotificationContainer *item = NULL;
		for (int32 i = 0; i < fAlerts.CountItems(); i++) {
			item = fAlerts.ItemAt(i);
			if (item->Priority() < container->Priority()) {
				fAlerts.InsertItem(container, i);
				inserted = true;
				break;
			}
		}
		if (!inserted)
			fAlerts.AddItem(container);
	}
	if (fAlerts.CountItems() == 1)
		NotifyListeners(B_PROPERTY_ADDED, "Next");
	outProperty = "B_OK";
	return B_OK;
}

void NotificationNode::Next(property &outProperty, const property_list &)
{
	outProperty.Undefine();

	if (fAlerts.CountItems() > 0) {
		outProperty = fAlerts.ItemAt(0L)->Copy();
		fAlerts.RemoveItem(0L);
		if (fAlerts.CountItems() > 0)
			NotifyListeners(B_PROPERTY_CHANGED, "Next");
	}
}

void NotificationNode::Peek(property &outProperty, const property_list &)
{
	if (fAlerts.CountItems() > 0)
		outProperty = fAlerts.ItemAt(0L)->Copy();
	else
		outProperty.Undefine();
}

void NotificationNode::Count(property &outProperty, const property_list &)
{
	outProperty = (int)(fAlerts.CountItems());
}

void NotificationNode::Clear()
{
	fAlerts.MakeEmpty();
}

void NotificationNode::PrintToStream()
{
	int32 count = fAlerts.CountItems();
	printf("* %ld alerts are in the queue.\n", count);
	for (int32 i = 0; i < count; i++) {
		NotificationContainer *container = fAlerts.ItemAt(i);
		printf("*\tAlert:%ld\n", i);
		printf("*\t\tcode = '%s'\n", container->Code().String());
		printf("*\t\ttimestamp = '%ld'\n", container->Timestamp());
		printf("*\t\tpriority = ");
		switch (container->Priority()) {
			case kNormalPriority:
				printf("normal.\n");
				break;
			case kAlphaPriority:
				printf("alpha.\n");
				break;
			case kBetaPriority:
				printf("beta.\n");
				break;
			case kCharliePriority:
				printf("charlie.\n");
				break;
			case kDeltaPriority:
				printf("delta.\n");
				break;
			default:
				printf("'%ld'.\n", container->Priority());
				break;
		}
		printf("*\t\tparameters = ");
		for (int32 j = 0; j < container->ParameterCount(); j++) {
			BinderNode::property prop;
			BinderNode::property_list args;
			BinderNode::property name = (int)(j);
			args.AddItem(&name);
			container->ReadProperty("GetParameter", prop, args);
			printf("'%s'", prop.String().String());
			if (j + 1 < container->ParameterCount())
				printf(",");
		}
		printf("\n");
	}
}

// -------------------------------------------------------------------
//							NotificationContainer
// -------------------------------------------------------------------
NotificationContainer::NotificationContainer()
	: 	BinderContainer(),
		fParameterCount(0)
{
	AddProperty("timestamp", time(NULL));
}

put_status_t NotificationContainer::WriteProperty(const char *name, const property &prop)
{
	return BinderContainer::WriteProperty(name, prop);
}

get_status_t NotificationContainer::ReadProperty(const char *name, property &prop, const property_list &args)
{
	
	if (strcmp(name, "AddParameter") == 0) {
		put_status_t result = B_OK;
		// Kinda kludgy, but there really isn't a way of having
		// multiple properties with the same name...
		BString tmp("param_");
		tmp << fParameterCount;
		result = AddProperty(tmp.String(), prop);
		if (result >= 0)
			fParameterCount++;
		return result.error;
	} else if (strcmp(name, "GetParameter") == 0) {
		int32 index = 0;
		if ((args.Count() > 0) && (args[0].Number() < fParameterCount))
			index = (int32)(args[0].Number());
		BString tmp("param_");
		tmp << index;
		return BinderContainer::ReadProperty(tmp.String(), prop);
	} else if (strcmp(name, "CountParameters") == 0) {
		prop = fParameterCount;
		return B_OK;
	}
	return BinderContainer::ReadProperty(name, prop, args);
}

BString NotificationContainer::Code()
{
	BinderNode::property prop;
	ReadProperty("code", prop);
	return (prop.String());
}

int32 NotificationContainer::Priority()
{
	BinderNode::property prop;
	ReadProperty("priority", prop);
	return (int32)(prop.Number());
}

int32 NotificationContainer::ParameterCount() const
{
	return fParameterCount;
}

time_t NotificationContainer::Timestamp()
{
	BinderNode::property prop;
	ReadProperty("timestamp", prop);
	return (int32)(prop.Number());
}


extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new NotificationNode();
}
