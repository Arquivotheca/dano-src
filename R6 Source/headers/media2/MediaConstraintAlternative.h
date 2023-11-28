/***************************************************************************
//
//	File:			media2/MediaConstraintAlternative.h
//
//	Description:	Stores a set of individual constraint items
//					(key->value relations) which are logically "and"ed
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIACONSTRAINTALTERNATIVE_H_
#define _MEDIA2_MEDIACONSTRAINTALTERNATIVE_H_

#include <support2/Vector.h>

#include <media2/MediaConstraintItem.h>

namespace B {
namespace Media2 {

using namespace Support2;

class BMediaFormat;

class BMediaConstraintAlternative
{
public:
									BMediaConstraintAlternative();
									BMediaConstraintAlternative(const BValue & archive);
		
	void							PrintToStream(ITextOutput::arg io) const;

	size_t							CountConstraintItems() const;
	const BMediaConstraintItem &	ConstraintItemAt (size_t i) const;

	BValue							AsValue() const;

private:

	BVector<BMediaConstraintItem> mConstraintItems;
	bool mConjunctive;

	friend class BMediaConstraint;
	
	BMediaConstraintItem::simplify_result_t Simplify();	
	void RemoveConstraintItemPair (size_t i, size_t j);					
	void AddConstraintItem (const BMediaConstraintItem &S);
	void AppendConstraintAlternative (const BMediaConstraintAlternative &C);		
};

}; }; // namespace B::Media2

#endif // _MEDIA2_MEDIACONSTRAINTALTERNATIVE_H_
