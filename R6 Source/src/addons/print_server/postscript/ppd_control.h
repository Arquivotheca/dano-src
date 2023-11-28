#ifndef _PPD_CONTROL_H_
#define _PPD_CONTROL_H_

#include <List.h>
#include <String.h>
#include "Element.h"

typedef enum { BOOLEAN, PICKONE, PICKMANY } UIType;

// forward declarations
class Group;
class BMessage;

class Constraint
{
 public:
						Constraint(const char *key1, const char *opt1,
									const char *key2, const char *opt2);
	virtual				~Constraint();
	
	virtual void		Assign();
 protected:
	BString				fKey1;
	BString				fOpt1;
	BString				fKey2;
	BString				fOpt2;
};

#if 0
class StringVal : public EList
{
 public:
						StringVal();
						~StringVal();
						
	const char*			Option() const;
	void				Option(const char*);

	const char*			Value() const;
	void				Value(const char*);
	
	void 				Print();
	
 private:
	BString				fOption;
	BString				fValue;
};
#endif

class Invocation : public EList
{
 public:
						Invocation(EList *parent);
	virtual				~Invocation();

	const char*			Option() const;
	void				Option(const char*);

	virtual const char*	Translation() const;
	void				Translation(const char*);

	void 				AddString(char*);
	
	char*				GetFirstString();
	char*				GetNextString();

	bool				IsDefault() const;
	void				IsDefault(bool);
	
	bool				IsEnabled() const;
	void				IsEnabled(bool);
	
	void 				Constrain(EList*);
	
	bool				IsConstrained() const;

	void				IsJCL(bool);
	bool				IsJCL() const;
	
	char*				GetEncodedName() const;
	
	void				Print();	

 private:
	EList*				fParent;
	BString				fOption;
	int32				fStringPtr;
	bool				fEnabled;
	bool				fDefault;
	bool				fIsJCL;
	BObjectList<char>	fStringList;
};						


class UI : public EList
{
 public:
						UI(Group *parent);
	virtual				~UI();
	
	Invocation*			CurrentInvocation() const;
	void				CurrentInvocation(Invocation*);

	UIType				InterfaceType() const;
	void				InterfaceType(UIType);

	const char*			Default() const;
	void				Default(const char*);

	void				EnableChild(EList *child);
	void				Close();
	
	EList*				Lookup(const char *name, const char *option);

	Invocation*			GetFirstInvoke();
	Invocation*			GetNextInvoke();

	bool				IsEnabled() const;
	void				IsEnabled(bool);

	bool				IsConflicted() const;

	void				CloseCurrentInvocation();
	
	void				AddInvocation(const char *name, const char *option, const char *translation);
	void				AddInvFragment(char *fragment);
		
	Invocation*			FindInvocation(const char *name, const char *option = NULL, int32 index = 0);

	void				ResetIndex();

	Group*				Parent() const;	

	void				MarkAsJCL();

	void				Print();	
	
 private:
	Invocation*			fCurrentInvocation;
	bool				fEnabled;
	UIType				fInterfaceType;
	int32				fIndex;
	Group*				fParent;
	BString				fDefault;
	EList*				fLastEnabled;
	bool				fIsJCL;
};

class Group : public EList
{
 public:
						Group(Group *parent);
	virtual				~Group();
						
	UI*					CurrentUI() const;
	void				CurrentUI(UI*);

	void				MarkCurrentUIAsJCL();
	
	void				CloseCurrentUI();

	virtual UI*			GetFirstUI();
	virtual UI*			GetNextUI();
	
	virtual void		ResetIndex();

	int32				HasInvocation(const char *name, const char *option = NULL);
	Invocation*			FindInvocation(const char *name, const char *option = NULL, int32 index = 0);

	void				AddInvocation(const char *name, const char *option, const char *translation);
	void				AddInvFragment(char *fragment);

	Group*				Parent() const;
	
	UI*					GetFirstJCL();
	UI*					GetNextJCL();
	
	void				Print();

 protected:
	int32				fUiIndex;
	int32				fJclIndex;

 private:	
	UI*					fCurrentUI;
	BObjectList<UI>		fJclUiList;
	Invocation*			fCurrentInvocation;
	
	Group*				fParent;
};

class PPD : public Group
{
 public:
	static PPD*			Instance(const BMessage *setup = NULL);
	virtual				~PPD();
	
	void				AddNewGroup(const char *name, const char *translation);
	void				CloseGroup(const char *name);
	
	void 				AddNewUI(const char *name, const char *translation, const char *type);
	void				CloseUI(const char *name);

	void				AddNewInvocation(const char *name, const char *option,
											const char *translation);	
	void				AddInvocationFragment(char *fragment);

	void				AddStringVal(const char *name, const char *option,
										const char *translation, const char *value);
										
	int					LanguageLevel();
	void				LanguageLevel(int);

	int32				Resolution();

	void				MakeDefault(const char *name, const char *option,
									const char *translation);
	
	void				AddUiConstraint(const char *key1, const char *opt1,
										const char *key2, const char *opt2);

	Group*				CurrentGroup() const;
	
	UI*					GetFirstUI();
	UI*					GetNextUI();
	UI*					GetUIAt(int32);
	UI*					GetUI(const char*);
		
	void				AssignConstraints();

	Invocation*			FindJCLHeaderInvocation();
	Invocation*			FindJCLFooterInvocation();
	Invocation*			FindJCLPSInvocation();

	void				Print();	
 private:
						PPD();

	void				SetValues(const BMessage*);

	static PPD*			fInstance;
	Group*				fCurrentGroup;
	int					fLanguageLevel;
	BObjectList<Constraint>		fUiConstraintList;
};

#endif
