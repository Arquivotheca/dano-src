#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include <String.h>
#include "ObjectList.h"
//#include "ppd_control.h"

typedef enum { _ELIST, _STRINGVAL, _INVOKE, _UI, _GROUP, _PPD } ClassType;

class EList : public BObjectList<EList>
{
 public:
						EList();
	virtual				~EList();

	const char*		Name() const;
	void			Name(const char*);

	virtual const char*		Translation() const;
	void					Translation(const char*);

	ClassType		WhatClass() const;
	void			WhatClass(ClassType);
	
	bool			IsList() const;

					
	EList*	 			Lookup(const char *name);

	virtual void		AddConstraint(EList *item);
	
	virtual void		Constrain(EList*);
	virtual void		Release(EList*);
	
	virtual bool		IsAvailable() const;
	virtual bool		IsConstrained() const;

	virtual void		Print();

 protected:
	BString			fName;
	BString			fTranslation;
	ClassType		fClass;

	BObjectList<EList>			fConstraints;
	BObjectList<EList>			fConstraintHolders;

};

#endif
