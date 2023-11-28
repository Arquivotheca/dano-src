#include "Element.h"
#include <stdio.h>

#define P //printf
#define PP //printf
#define FPF //fprintf

const char*
EList::Name() const
{
	return fName.String();
}

void
EList::Name(const char* name)
{
	P("***Element::Name(%s)\n", name);
	fName = name;
	P("\tfName.String = [%s]\n", fName.String());
}

const char*
EList::Translation() const
{
	// if no translation was given, just return the name
	if(fTranslation.Length() == 0){
		return fName.String();
	} else {
		return fTranslation.String();
	}
}

void
EList::Translation(const char* trans)
{
	fTranslation = trans;
}

bool
EList::IsList() const
{
	if((fClass == _GROUP) || (fClass == _PPD))
		return true;
	else
		return false;
}

ClassType
EList::WhatClass() const
{
	return fClass;
}

void
EList::WhatClass(ClassType type)
{
	fClass = type;
}


EList::EList()
	: BObjectList()
{
	FPF(stderr,"New EList\n");
	WhatClass(_ELIST);
}

EList::~EList()
{
	FPF(stderr,"Delete EList: %s %s\n", Name(), Translation());
	EList *item;
	int32 count = CountItems();
	for(int32 i=0; i < count; i++){
		item = RemoveItemAt(0);
		delete item;
	}
	
	fConstraints.MakeEmpty();
	fConstraintHolders.MakeEmpty();
}

EList*
EList::Lookup(const char* name)
{
	EList *e;
	for(int i=0; (e = (EList*)ItemAt(i)); i++){
		if(!strcmp(name, e->Name()))
			return e;
	}	
	return NULL;
}

void
EList::AddConstraint(EList *item)
{
	fConstraints.AddItem(item);
}

void
EList::Constrain(EList *holder)
{
	fConstraintHolders.AddItem(holder);
}

void
EList::Release(EList *holder)
{
	fConstraintHolders.RemoveItem(holder);
}


bool
EList::IsAvailable() const
{
	return (fConstraintHolders.CountItems() == 0);
}

bool
EList::IsConstrained() const
{
	return (fConstraintHolders.CountItems() > 0);
}

void
EList::Print()
{
	return;
}
