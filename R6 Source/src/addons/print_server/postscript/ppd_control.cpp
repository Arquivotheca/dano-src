#include "ppd_control.h"
#include <string.h>
#include <stdio.h>
#include <Alert.h>
#include <Message.h>
#include <stdlib.h>

#define P // printf
#define PP // printf
#define FPF  //fprintf

// give storage to those statics!
PPD* PPD::fInstance = NULL;

UIType get_ui_type_for_name(const char *name)
{
	P("get_ui_type_for_name(%s)\n", name);
	if(!strcasecmp(name, "boolean")){
		return BOOLEAN;
	} else if(!strcasecmp(name, "pickone")) {
		return PICKONE;
	} else {
		return PICKMANY;
	}	
}

char*
bracketed_ascii_to_hex(const char *in)
{
	if(in == NULL) return NULL;
	P("bracked_ascii_to_hex(%s)\n", in);
	char *out = new char[1024];	// much larger than allowable PPD lines.
	char *start_of_out = out;
	char tmp[3];
	bool hex_mode = false;
	while(*in != '\0'){
		if(!hex_mode){
			if(*in == '<'){
				hex_mode = true;
				in++;
			} else {
				*out = *in;
				out++; in++;
			}
		} else {
			if(*in == '>'){
				hex_mode = false;
				in++;
			} else {
				strncpy(tmp,in,2);
				tmp[2] = '\0';	
				*out = (uchar)strtoul(tmp, NULL, 16);
				out++;
				in += 2;
			}
		}
	}

	*out = '\0';
	P("\n\toutstring: [%s]\n", start_of_out);
	return start_of_out;
}

/****************************************************/
Constraint::Constraint(const char *key1, const char *opt1,
									const char *key2, const char *opt2)
{
	FPF(stderr,"New Constraint: %s %s %s %s\n", key1, opt1, key2, opt2);
	fKey1 = key1;
	if(opt1) fOpt1 = opt1;
	fKey2 = key2;
	if(opt2) fOpt2 = opt2;
}

Constraint::~Constraint()
{
	FPF(stderr,"Delete Constraint: %s %s %s %s\n", fKey1.String(), fOpt1.String(), fKey2.String(), fOpt2.String());
	// nothing yet
}

void
Constraint::Assign()
{
	PPD *ppd = PPD::Instance();
	
	EList *item1;
	EList *item2;
	
	P("\t\tfOpt1.String() == %s\n", fOpt1.String());
	if(fOpt1.Length() == 0){
		item1 = ppd->GetUI(fOpt1.String());
	} else {
		item1 = ppd->FindInvocation(fKey1.String(), fOpt1.String());
		P("Constraint::Assign: FindInvocation(%s,%s) = 0x%x\n",
			fKey1.String(), fOpt1.String(), item1);
	}	

	P("\t\tfOpt2.String() == %s\n", fOpt2.String());
	if(fOpt2.Length() == 0){
		item2 = ppd->GetUI(fOpt2.String());
	} else {
		item2 = ppd->FindInvocation(fKey2.String(), fOpt2.String());
		P("Constraint::Assign: FindInvocation(%s,%s) = 0x%x\n",
			fKey2.String(), fOpt2.String(), item2);
	}
	
	// don't continue if a constrainee doesn't exist.
	if(item1 == NULL || item2 == NULL) return;

	// according to the PPD spec, these should be reciprocal
	P("Item %s %s now constrains Item %s %s\n", fKey1.String(), (fOpt1.Length()==0)?"":fOpt1.String(),
				fKey2.String(), (fOpt2.Length()==0)?"":fOpt2.String());
	item1->AddConstraint(item2);
	P("Item %s %s now constrains Item %s %s\n", fKey2.String(), (fOpt2.Length()==0)?"":fOpt2.String(),
				fKey1.String(), (fOpt1.Length()==0)?"":fOpt1.String());
	item2->AddConstraint(item1);
}

/****************************************************/
#if 0
StringVal::StringVal()
{
	WhatClass(_STRINGVAL);
}

StringVal::~StringVal()
{
	// nothing yet
}

const char*
StringVal::Option() const
{
	return fOption.String();
}

void
StringVal::Option(const char *option)
{
	fOption = option;
}

const char*
StringVal::Value() const
{
	return fValue.String();
}

void
StringVal::Value(const char *value)
{
	fValue = value;
}

void
StringVal::Print()
{
	FPF(stderr,"\n\t\tInvocation Name: %s\n", Name());
	FPF(stderr,"\t\tInvocation Option: %s\n", Option());
	FPF(stderr,"\t\tInvocation Translation: %s\n", Translation());
	FPF(stderr,"\n%s\n", fValue.String());
}
#endif
/****************************************************/

Invocation::Invocation(EList *parent)
{
	FPF(stderr,"New Invocation\n");
	
	WhatClass(_INVOKE);
	fParent = parent;
	fStringPtr = 0;
	fDefault = false;
	fEnabled = false;
	fIsJCL = false;
}

Invocation::~Invocation()
{
	FPF(stderr,"Delete Invocation: %s %s %s\n", Name(), Option(), Translation());
	char *str;
	int32 count = fStringList.CountItems();
	//FPF(stderr,"In ~Invocation(), %s %s\n", Name(), Option());
	for(int32 i = 0; i < count; i++){
		str = fStringList.RemoveItemAt(0);
		//fprintf(stderr,"delete string: [%s]\n", str);
		free(str);
	}
}

const char*
Invocation::Option() const
{
	return fOption.String();
}

void
Invocation::Option(const char *option)
{
	fOption = option;
}

const char*
Invocation::Translation() const
{
	if(fTranslation.Length() == 0){
		return fOption.String();
	} else {
		return fTranslation.String();
	}
}

void
Invocation::Translation(const char *trans)
{
	EList::Translation(trans);
}

void
Invocation::AddString(char *str)
{
	//FPF(stderr,"Invocation::AddString(), %s %s\n", Name(), Option());
	//FPF(stderr,"\t[%s]\n", str);
	fStringList.AddItem(str);
}

char*
Invocation::GetFirstString()
{
	fStringPtr = 0;
	return GetNextString();
}

char*
Invocation::GetNextString()
{
	if(!IsJCL()){
		char *original = fStringList.ItemAt(fStringPtr++);
		if(original == NULL){
			return NULL;
		}
		char *dup = new char[strlen(original)+1];
		sprintf(dup, original);
		return dup;
	}
	
	char *converted;
	converted = bracketed_ascii_to_hex(fStringList.ItemAt(fStringPtr++));
	return converted;
}

bool
Invocation::IsEnabled() const
{
	return fEnabled;
}

void
Invocation::IsEnabled(bool enable)
{
	if(enable){
		P("\t\t* Enabling: %s %s\n", Name(), Option());
	} else {
		P("\t\t* Disabling: %s %s\n", Name(), Option());
	}
	
	fEnabled = enable;
	if(enable && fParent->WhatClass() == _UI){
		dynamic_cast<UI*>(fParent)->EnableChild(this);
	}

	/* now walk through the list of constraints, and enforce */
	/* or clear them depending on the enable flag */
	EList *e;		
	P("Invocation %s %s\n", Name(), (Option())?Option():"");
	if(enable){
		for(int i=0; (e=fConstraints.ItemAt(i)); i++){
			e->Constrain(this);
		}
	} else {
		for(int i=0; (e=fConstraints.ItemAt(i)); i++){
			e->Release(this);
		}
	}	
}

bool
Invocation::IsDefault() const
{
	if(fParent->WhatClass() == _UI){
		if(!strcmp(fOption.String(), dynamic_cast<UI*>(fParent)->Default())){
			return true;
		}
	}
	return false;
}

void
Invocation::IsDefault(bool val)
{
	fDefault = val;
	if(val) { fEnabled = true; }
}

void
Invocation::Constrain(EList *holder)
{
	// diagnostic only.  Remove this function!!
	P("\tis constraining %s %s.\n", Name(), (Option())?Option():"");

	EList::Constrain(holder);
}

bool
Invocation::IsConstrained() const
{
	P("%s %s is constrained by:\n", Name(), (Option())?Option():"");
	Invocation *inv;
	if(fConstraintHolders.CountItems() == 0){
		P("\t\t(Nothing.)\n");
	} else {
		for(int32 i=0; (inv=dynamic_cast<Invocation*>(fConstraintHolders.ItemAt(i))); i++){
			P("\t\t%s %s.\n", inv->Name(), (inv->Option())?inv->Option():"");
		}
	}
	return EList::IsConstrained();
}

void
Invocation::IsJCL(bool val)
{
	fIsJCL = val;
}

bool
Invocation::IsJCL() const
{
	if(!strncmp(Name(), "JCL", 3)) { return true; }
	return fIsJCL;
}

char*
Invocation::GetEncodedName() const
{
	BString ename = Name();
	ename << "|" << Option();
	P("Invocation::GetEncodedName() = %s\n", ename.String());
	return strdup(ename.String());
}

void
Invocation::Print()
{
	const char *str;
	FPF(stderr,"\n\t\tInvocation Name: %s\n", Name());
	FPF(stderr,"\t\tInvocation Option: %s\n", Option());
	FPF(stderr,"\t\tInvocation Translation: %s\n", Translation());
	for(int i=0; (str=fStringList.ItemAt(i)); i++){
		FPF(stderr,"\t\t%s\n", str);
	}
}

/****************************************************/

UI::UI(Group *parent)
{
	FPF(stderr,"New UI\n");
	fParent = parent;
	fCurrentInvocation = NULL;
	fLastEnabled = NULL;
	fIsJCL = false;
	WhatClass(_UI);
}

UI::~UI()
{
	FPF(stderr,"Delete UI: %s %s\n", Name(), Translation());
	// handled by EList::~EList()
}

Invocation*
UI::CurrentInvocation() const
{
	return fCurrentInvocation;
}

void
UI::CurrentInvocation(Invocation *inv)
{
	fCurrentInvocation = inv;
}

void
UI::CloseCurrentInvocation()
{
	fCurrentInvocation = NULL;
}

UIType
UI::InterfaceType() const
{
	return fInterfaceType;
}

void
UI::InterfaceType(UIType type)
{
	fInterfaceType = type;
	if(type == PICKMANY){
		P("InterfaceType for %s is PICKMANY!\n", Name());
	}
}

const char*
UI::Default() const
{
	return fDefault.String();
}

void
UI::Default(const char *def)
{
	P("UI::Default(%s) 0x%x\n", (def)?def:"NULL", def);
	fDefault = def;
}

void
UI::Close()
{
	Invocation *inv;
	P("UI::Close: inv = FindInvocation(%s, %s)\n", Name(), fDefault.String());
	inv = FindInvocation(Name(), fDefault.String());
	if(inv) {
		P("UI::Close: Default invocation found! (%s, %s)\n", inv->Name(), inv->Option());
		inv->IsEnabled(true);
	}
}

void
UI::EnableChild(EList *child)
{
	P("EnabledChild(%s)\n", (child->Name())?child->Name():"NULL");
	if(child == fLastEnabled) { return; }
	switch(fInterfaceType)
	{
	 case BOOLEAN:
	 case PICKONE:
		P("fLastEnabled = 0x%x\n", fLastEnabled);
		if(fLastEnabled) {
			dynamic_cast<Invocation*>(fLastEnabled)->IsEnabled(false);
		}
		fLastEnabled = child;
		break;
	 case PICKMANY:
		fLastEnabled = child;
		break;		
	}
}

bool
UI::IsEnabled() const
{
	return fEnabled;
}

void
UI::IsEnabled(bool val)
{
	fEnabled = val;

	/* walk through the list of constraints, and enforce */
	/* or clear them depending on the enable flag */
	EList *e;
		
	if(val){
		for(int i=0; (e=fConstraints.ItemAt(i)); i++){
			dynamic_cast<Invocation*>(e)->Constrain(this);
		}
	} else {
		for(int i=0; (e=fConstraints.ItemAt(i)); i++){
			dynamic_cast<Invocation*>(e)->Release(this);
		}
	}
}

bool
UI::IsConflicted() const
{
	if(fLastEnabled && fLastEnabled->IsConstrained()){
		return true;
	}

	return false;
}

EList*
UI::Lookup(const char *name, const char *option)
{
	Invocation *inv;
	for(int i=0; (inv=dynamic_cast<Invocation*>(ItemAt(i))); i++){
		if(strcmp(inv->Name(), name) && strcmp(inv->Option(), option)){
			return inv;
		}	
	}
	return inv;
}

Invocation*
UI::GetFirstInvoke()
{
	ResetIndex();
	return GetNextInvoke();
}

Invocation*
UI::GetNextInvoke()
{
	return dynamic_cast<Invocation*>(ItemAt(fIndex++));
}

Invocation*
UI::FindInvocation(const char *name, const char *option, int32 index)
{
	EList *e;
	Invocation *inv;
	int itemCount = 0;
	for(int i=0; (e=ItemAt(i)); i++){
		switch(e->WhatClass())
		{
		 case _INVOKE:
			inv = dynamic_cast<Invocation*>(e);
			//P("Comparing [%s,%s] to arguments: [%s,%s]\n", inv->Name(), inv->Option(), name, option);
			if(!strcmp(inv->Name(), name) &&
				(!option || !strcmp(inv->Option(), option))){
				if(index == itemCount){
					return inv;
				} else {
					itemCount++;
				}
			}
			break;
		 default:
			break;
		}
	}
	return NULL;
}

void
UI::ResetIndex()
{
	fIndex = 0;
}

void
UI::AddInvocation(const char *name, const char *option, const char *translation)
{
	Invocation *inv;
	inv = dynamic_cast<Invocation*>(Lookup(name, option));
	if(inv == NULL){
		P("Invocation not found, adding new Invocation.\n");
		inv = new Invocation(this);
		inv->Name(name);
		inv->Option(option);
		AddItem(inv);
	}	
	inv->Translation(translation);
	P("Invocation is now [%s][%s][%s]\n", inv->Name(), inv->Option(),
				inv->Translation());
	CurrentInvocation(inv);
	inv->IsJCL(fIsJCL);
}

void
UI::AddInvFragment(char *fragment)
{
	if(!fCurrentInvocation)
		return;
		
	fCurrentInvocation->AddString(fragment);
}

Group*
UI::Parent() const
{
	return fParent;
}

void
UI::MarkAsJCL()
{
	fIsJCL = true;
}

void
UI::Print()
{
	FPF(stderr,"\tUI Name: %s\n", (Name()==NULL)?"Null!":Name());
	FPF(stderr,"\tUI Translation: %s\n", (Translation()==NULL)?"Null!":Translation());
	
	Invocation *inv;
	for(inv=GetFirstInvoke(); inv; inv=GetNextInvoke()){
		inv->Print();
	}
}

/****************************************************/

Group::Group(Group *parent)
{
	FPF(stderr,"New Group\n");
	fParent = parent;
	fCurrentUI = NULL;
	fCurrentInvocation = NULL;
	WhatClass(_GROUP);
}

Group::~Group()
{
	FPF(stderr,"Delete Group: %s %s\n", Name(), Translation());
	fJclUiList.MakeEmpty();
}

UI*
Group::CurrentUI() const
{
	return fCurrentUI;
}

void
Group::CurrentUI(UI *ui)
{
	fCurrentUI = ui;
}

void
Group::CloseCurrentUI()
{
	if(fCurrentUI) { fCurrentUI->Close(); }
	fCurrentUI = NULL;
}

UI*
Group::GetFirstUI()
{
	P("Group::GetFirstUI\n");
	ResetIndex();
	return GetNextUI();
}

UI*
Group::GetNextUI()
{
	P("Group::GetNextUI, index = %d\n", fUiIndex);
	return dynamic_cast<UI*>(ItemAt(fUiIndex++));
}

void
Group::MarkCurrentUIAsJCL()
{
	if(fCurrentUI){
		fCurrentUI->MarkAsJCL();
		fJclUiList.AddItem(fCurrentUI);
	}
}


void
Group::ResetIndex()
{
	fUiIndex = 0;
	P("Reset: Group %s, fUiIndex: %d\n", Name(), fUiIndex);
	EList *e;
	for(int i=0; (e=ItemAt(i)); i++){
		if(e->WhatClass() == _GROUP){
			dynamic_cast<Group*>(e)->ResetIndex();
		} else if(e->WhatClass() == _UI) {
			dynamic_cast<UI*>(e)->ResetIndex();
		}
	}
}

#if 0
EList*
Group::Lookup(const char *name, const char *option)
{
	EList *e;
	for(int i=0; e=(EList*)ItemAt(i); i++){
		switch(e->WhatClass()){
			case _STRINGVAL:
			{
				StringVal *sv = (StringVal*)e;
				if(!strcmp(sv->Name(), name) && !strcmp(sv->Option(), option)){
					return sv;
				}
				break;
			}
			
		}		/* switch */
	} 			/* for */
	return NULL;
}
#endif

int32
Group::HasInvocation(const char *name, const char *option)
{
	if(name == NULL){
		return 0;
	}
	
	EList *e;
	int32 hasCount = 0;
	for(int i=0; (e=(EList*)ItemAt(i)); i++){
		//P("Checking Name: [%s]\n", e->Name());
		if(e->WhatClass() == _INVOKE){
			Invocation *inv = dynamic_cast<Invocation*>(e);
			//P("Checking Option: [%s]\n", inv->Option());
			if(!strcmp(inv->Name(), name) &&
				(!option || !strcmp(inv->Option(), option))){
				hasCount++;
			}
		}
	}
	return hasCount;
}

Invocation*
Group::FindInvocation(const char *name, const char *option, int32 index)
{
	if(name == NULL) {
		return NULL;
	}

	EList *e;
	Invocation *inv;
	int itemCount = 0;
	for(int i=0; (e=ItemAt(i)); i++){
		switch(e->WhatClass())
		{
		 case _GROUP:
			inv = dynamic_cast<Group*>(e)->FindInvocation(name, option, index);
			if(inv) return inv;
			break;		
		 case _UI:
			inv = dynamic_cast<UI*>(e)->FindInvocation(name, option, index);
			if(inv) return inv;
			break;		
		 case _INVOKE:
			inv = dynamic_cast<Invocation*>(e);
			//P("\tFindInvocation: Checking against: [%s, %s]\n", inv->Name(), inv->Option());
			if(!strcmp(inv->Name(), name) &&
				(!option || !strcmp(inv->Option(), option))){
				if(index == itemCount){
					return inv;
				} else {
					itemCount++;
				}
			}
			break;
		 default:
			break;
		}
	}
	return NULL;
}

void
Group::AddInvocation(const char *name, const char *option, const char *translation)
{
	if(HasInvocation(name, option) != 0){
		P("What?!  We already name Invocation(%s,%s,%s)!\n", name, option, translation);
		return;
	}
		
	Invocation *inv = new Invocation(this);
	inv->Name(name);
	inv->Option(option);
	inv->Translation(translation);

	P("Group::AddInvocation: Adding Invocation(%s,%s).\n", name, option);	
	AddItem(inv);
	fCurrentInvocation = inv;
}

void
Group::AddInvFragment(char *fragment)
{
	if(!fCurrentInvocation)
		return;
		
	fCurrentInvocation->AddString(fragment);
}

Group*
Group::Parent() const
{
	return fParent;
}

UI*
Group::GetFirstJCL()
{
	fJclIndex = 0;
	return GetNextJCL();
}

UI*
Group::GetNextJCL()
{
	EList *e = fJclUiList.ItemAt(fJclIndex);
	while(e && e->WhatClass() == _INVOKE){
		e = ItemAt(++fJclIndex);
	}
	
	if(!e) { return NULL; }

	if(e->WhatClass() == _GROUP){
		P("It's a list.\n");
		UI *lower = dynamic_cast<Group*>(e)->GetNextJCL();
		if(!lower){
			fJclIndex++;
			return GetNextJCL();
		}
		return lower;
	} 

	// non-list item found...	
	fJclIndex++;
	return dynamic_cast<UI*>(e);			
}


void
Group::Print()
{
	EList *e;
	for(int i=0; (e=ItemAt(i)); i++){
		e->Print();
	}
}
	
/****************************************************/


PPD*
PPD::Instance(const BMessage *setup)
{
	if(fInstance == NULL){
		FPF(stderr,"PPD::Instance(): need to create a new PPD object...\n");
		fInstance = new PPD;
	} else {
		FPF(stderr,"PPD::Instance(): using existing PPD object...\n");	
	}

	if(fInstance && setup) {
		fInstance->SetValues(setup);
	}
	
	return fInstance;	
}

PPD::PPD()
	: Group(this)
{
	FPF(stderr,"In the PPD constructor!\n");
	fCurrentGroup = this;
	WhatClass(_PPD);
	Name("TopList");
	Translation("General Options");
	fLanguageLevel = 1;
	
	FPF(stderr,"New PPD: %s %s\n", Name(), Translation());

}

PPD::~PPD()
{
	FPF(stderr,"Delete PPD: %s %s\n", Name(), Translation());
	fInstance = NULL;
	Constraint *constraint;
	int32 count = fUiConstraintList.CountItems();
	for(int32 i=0; i < count; i++){
		constraint = fUiConstraintList.RemoveItemAt(0);
		delete constraint;
	}
}

Group*
PPD::CurrentGroup() const
{
	return fCurrentGroup;
}

void
PPD::AddNewGroup(const char *name, const char *translation)
{
	P("AddNewGroup: [%s][%s]\n", name, translation);
	Group *g;
	g = dynamic_cast<Group*>(EList::Lookup(name));
	if(g == NULL){
		P("Group not found, adding new group.\n");
		g = new Group(this);
		g->Name(name);
		AddItem(g);
	}
	g->Translation(translation);
	P("Group is now: [%s][%s]\n", g->Name(), g->Translation());
	P("TopList now has %d elements.\n", CountItems());
	fCurrentGroup = g;
}

void 
PPD::CloseGroup(const char *name)
{
	if(strcmp(CurrentGroup()->Name(), name)){
		BString str = "CloseGroup argument doesn't match name of ";
		str << "CurrentGroup.  Check the PPD.";
		(new BAlert("", str.String(), "Ugh!"))->Go();
	}
	fCurrentGroup = this;	
}

void
PPD::AddNewUI(const char *name, const char *translation, const char *type)
{
	P("AddNewUI: [%s][%s][%s]\n", name, translation, type);
	Group *group = CurrentGroup();
	P("addnewUI: CurrentGroup name: [%s]\n", (group->Name())?group->Name():"NULL");
	UI *ui = dynamic_cast<UI*>(group->EList::Lookup(name));
	if(ui == NULL){
		P("UI not found, adding new UI.\n");
		ui = new UI(group);
		ui->Name(name);
		group->AddItem(ui);
	}
	ui->Translation(translation);
	ui->InterfaceType(get_ui_type_for_name(type));
	P("UI is now: [%s][%s][%d]\n", ui->Name(), ui->Translation(), ui->InterfaceType());
	P("TopList now has %d elements.\n", CountItems());
	CurrentGroup()->CurrentUI(ui);
}

void
PPD::CloseUI(const char *name)
{
	if(strcmp(CurrentGroup()->CurrentUI()->Name(), name)){
		BString str = "CloseUI argument doesn't match name of ";
		str << "CurrentUI.  Check the PPD.";
		(new BAlert("", str.String(), "Ugh!"))->Go();
	}
	CurrentGroup()->CloseCurrentUI();	
}

void
PPD::AddNewInvocation(const char *name, const char *option,
						const char *translation)
{
	P("--- AddNewInvoke: [%s][%s][%s]\n", name, option, translation);
	UI *ui = CurrentGroup()->CurrentUI();
	if(ui) {
		P("PPD::AddNewInvocation: Adding the invocation (%s, %s) to the current UI.\n",
			name, option);
		ui->AddInvocation(name, option, translation);
		return;
	} else {
		P("PPD::AddNewInvocation: Calling Group::AddInvocation()...\n");
		Group::AddInvocation(name, option, translation);
	}

}

void
PPD::AddInvocationFragment(char *fragment)
{
	UI *ui = CurrentGroup()->CurrentUI();
	if(ui){
		ui->AddInvFragment(fragment);
	} else {
		CurrentGroup()->AddInvFragment(fragment);
	}
}

void
PPD::AddStringVal(const char *name, const char *option,
					const char *translation, const char *value)
{
#if 0
	Group *g = CurrentGroup();
	StringVal *sv;
	sv = static_cast<StringVal*>(g->Lookup(name, option));
	if(sv == NULL){
		sv = new StringVal;
		sv->Name(name);
		sv->Option(option);
		g->AddItem(sv);
	}
	sv->Translation(translation);
	sv->Value(value);
#endif
}

int
PPD::LanguageLevel()
{
	if(HasInvocation("LanguageLevel")){
		Invocation *inv = FindInvocation("LanguageLevel");
		char *str = inv->GetFirstString();
		int level = atoi(str);
		delete str;
		return level;
	} else {
		return fLanguageLevel;
	}
}

void
PPD::LanguageLevel(int lev)
{
	fLanguageLevel = lev;
}

int32
PPD::Resolution()
{
	Invocation *inv = NULL;
	Invocation *enabledRes = NULL;
	Invocation *defaultRes = NULL;

	UI *resUI = GetUI("Resolution");
	if(resUI) {
		for(inv=resUI->GetFirstInvoke(); inv; inv=resUI->GetNextInvoke()) {
			if(inv->IsDefault()) {
				defaultRes = inv;
			}
			if(inv->IsEnabled()) {
				enabledRes = inv;
				break;
			}
		}
	} else {
		int32 resCount = HasInvocation("Resolution");
		if(resCount) {
			for(int32 resNum=0; resNum < resCount; resNum++) {
				inv = FindInvocation("Resolution", NULL, resNum);
				if(inv->IsDefault()) {
					defaultRes = inv;
				}
				if(inv->IsEnabled()) {
					enabledRes = inv;
					break;
				}
			}
		}
	}
	
	int32 resolution;
	if(enabledRes) {
		resolution = atoi(enabledRes->Option());
	} else if (defaultRes) {
		resolution = atoi(defaultRes->Option());
	} else {
		resolution = 300;
	}
	
	printf("Resolution is set to %ld dpi.\n", resolution);
	return resolution;
}

void
PPD::MakeDefault(const char *name, const char *option, const char *translation)
{
	P("PPD::MakeDefault(%s, %s)\n", (option)?option:"NULL",
				(translation)?translation:"NULL");
	if(!option) return;
	UI *ui = fCurrentGroup->CurrentUI();
	if(ui == NULL){
		P("no UI for fCurrentGroup: [%s]\n", fCurrentGroup->Name());
		AddNewInvocation(name, option, translation);
		Invocation *inv = FindInvocation(name, option);
		inv->IsDefault(true);	
		return;
	} else {
		ui->Default(option);
	}
}

void
PPD::AddUiConstraint(const char *key1, const char *opt1, const char *key2,
						const char *opt2)
{
	P("Adding new UI constraint: (%s,%s,%s,%s)\n", key1, opt1, key2, opt2);
	Constraint *item = new Constraint(key1, opt1, key2, opt2);
	fUiConstraintList.AddItem(item);
}


UI*
PPD::GetFirstUI()
{
	P("PPD::GetFirstUI.\n");
	ResetIndex();
//	fUiIndex = 0;
	return GetNextUI();
}

UI*
PPD::GetNextUI()
{
	P("PPD::GetNextUI, index = %d\n", fUiIndex);
	EList *e = ItemAt(fUiIndex);
	while(e && e->WhatClass() ==_INVOKE){
		e = ItemAt(++fUiIndex);
	}
	
	if(!e){	return NULL; }

	P("e->WhatClass: %d\n", e->WhatClass());	
	if(e->WhatClass() == _GROUP){
		P("It's a list.\n");
		UI *lower = dynamic_cast<Group*>(e)->GetNextUI();
		if(!lower){
			fUiIndex++;
			return GetNextUI();
		}
		return lower;
	} 

	// non-list item found...	
	fUiIndex++;
	return dynamic_cast<UI*>(e);		
}

UI*
PPD::GetUIAt(int32 index)
{
	ResetIndex();
	if(index != 0){
		UI *ui;
		for(int i = 0; i < index; i++){
			ui = GetNextUI();
		}
	}
	return GetNextUI();
}

UI*
PPD::GetUI(const char *name)
{
	UI *ui;
	for(ui=GetFirstUI(); ui; ui=GetNextUI()){
		if(!strcmp(name, ui->Name()))
			return ui;
	}
	return NULL;
}

void
PPD::AssignConstraints()
{
	Constraint *c;
	int32 countItems = fUiConstraintList.CountItems();
	P("AssignConstraints, countItems = %d\n", countItems);
	for(int i=0; i < countItems; i++){
		c = fUiConstraintList.RemoveItemAt(0);
		c->Assign();
		delete c;
	}
}

void
PPD::SetValues(const BMessage *setup)
{
	if(setup == NULL) { return; }
	
	const char *name;
	char *dup_name;
	uint32 type;
	int32 count;
	char *keyword;
	char *option;

	if(setup->HasMessage("ppd_item")){
		BMessage ppdMsg;
		setup->FindMessage("ppd_item", &ppdMsg);
	
		// handle options set in the "Advanced Option" window
		for(int32 i=0;
			ppdMsg.GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK;
			i++){
		
			FPF(stderr,"SetValues: name: %s\n", name);
			dup_name = strdup(name);
			char *save_ptr;
			keyword = strtok_r(dup_name, "|", &save_ptr);
			option = strtok_r(NULL, "|", &save_ptr);
			FPF(stderr,"Found keyword: [%s], Option: [%s]\n", keyword, option);			
				
			Invocation *inv = FindInvocation(keyword, option);
			if(inv == NULL) {
				free (dup_name);
				continue;
			}
	
			inv->IsEnabled(true);
			free (dup_name);
		}
	}
			
	// now handle options which are set in the regular setup window
	// These need to have reciprocol actions in TSetupView
	if(setup->HasString("page size")){
		const char *page_size = setup->FindString("page size");

		Invocation *inv = FindInvocation("PageSize", page_size);
		if(inv) { inv->IsEnabled(true); }

		inv = FindInvocation("PageRegion", page_size);
		if(inv) { inv->IsEnabled(true); }		
	}
	
}

Invocation*
PPD::FindJCLHeaderInvocation()
{
	return FindInvocation("JCLBegin");
}

Invocation*
PPD::FindJCLFooterInvocation()
{
	return FindInvocation("JCLEnd");
}

Invocation*
PPD::FindJCLPSInvocation()
{
	return FindInvocation("JCLToPSInterpreter");
}

void
PPD::Print()
{
	EList *e;
	for(int i=0; (e=ItemAt(i)); i++){
		e->Print();
	}
	
//	for(UI *ui=GetFirstUI(); ui; ui=GetNextUI()){
//		ui->Print();
//	}
}

