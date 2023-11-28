/***************************************************************************
//
//	File:			media2/MediaConstraint.h
//
//	Description:	A flexible representation of format requirements
//					(which may be wildcards or constraints with exact values.)
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIACONSTRAINT_H_
#define _MEDIA2_MEDIACONSTRAINT_H_

#include <media2/MediaFormat.h>

#include <support2/Value.h>
#include <support2/Vector.h>

#include <media2/MediaDefs.h>
#include <media2/MediaConstraintAlternative.h>

namespace B {
namespace Media2 {

using namespace Support2;

// BMediaConstraint represents a constraint on a number of "keys" related to "values".
// It is composed of alternatives, each of which is a term in CNF (conjunctive normal form)
// Sy := [!](k1 o v1) && [!](k2 o v2) && ... && [!](kx o vx)
// The whole expression is then formed by "or"ing these alternatives:
// S1 || S2 || S3 || ... || Sm

class BMediaConstraint
{
public:
								BMediaConstraint(bool value = true);
					
								BMediaConstraint(const BMediaConstraintItem & S);

								// implicit '=='
								BMediaConstraint(
									const BValue & key,
									const BValue & value,
									BMediaConstraintItem::constraint_scope_t scope
										= BMediaConstraintItem::B_MANDATORY);

								BMediaConstraint(
									const BValue & key,
									BMediaConstraintItem::relation_t rel,
									const BValue & value,
									BMediaConstraintItem::constraint_scope_t scope
										= BMediaConstraintItem::B_MANDATORY);
			
								BMediaConstraint(
									const BValue & key,
									const BValue & from,
									const BValue & to,
									bool from_inclusive = true,
									bool to_inclusive = true,
									BMediaConstraintItem::constraint_scope_t scope
										= BMediaConstraintItem::B_MANDATORY);

								BMediaConstraint(const media_format & format);

								BMediaConstraint(const BMediaFormat & format);
								
								BMediaConstraint(const BValue & archive);
									
			BMediaConstraint &	And(const BMediaConstraint &constraint);
			
	inline	BMediaConstraint &	And(const BValue & key,
									const BValue & value,
									BMediaConstraintItem::constraint_scope_t scope
										= BMediaConstraintItem::B_MANDATORY)
								{ return And(BMediaConstraint(key, value,scope)); }

	inline	BMediaConstraint &	And(const BValue & key,
									BMediaConstraintItem::relation_t rel,
									const BValue & value,
									BMediaConstraintItem::constraint_scope_t scope
										= BMediaConstraintItem::B_MANDATORY)
								{ return And(BMediaConstraint(key, rel, value,scope)); }

	inline	BMediaConstraint &	And(const BValue & key,
									const BValue & from,
									const BValue & to,
									bool from_inclusive = true,
									bool to_inclusive = true,
									BMediaConstraintItem::constraint_scope_t scope
										= BMediaConstraintItem::B_MANDATORY)
								{ return And(BMediaConstraint(key, from, to, from_inclusive, to_inclusive,scope)); }

			BMediaConstraint &	Or( const BMediaConstraint &constraint);

	inline	BMediaConstraint &	Or( const BValue & key,
									const BValue & value,
									BMediaConstraintItem::constraint_scope_t scope
										= BMediaConstraintItem::B_MANDATORY)
								{ return Or(BMediaConstraint(key, value,scope)); }

	inline	BMediaConstraint &	Or( const BValue & key,
									BMediaConstraintItem::relation_t rel,
									const BValue & value,
									BMediaConstraintItem::constraint_scope_t scope
										= BMediaConstraintItem::B_MANDATORY)
								{ return Or(BMediaConstraint(key, rel, value,scope)); }
		
	inline	BMediaConstraint &	Or( const BValue & key,
									const BValue & from,
									const BValue & to,
									bool from_inclusive = true,
									bool to_inclusive = true,
									BMediaConstraintItem::constraint_scope_t scope
										= BMediaConstraintItem::B_MANDATORY)
								{ return Or(BMediaConstraint(key, from, to, from_inclusive, to_inclusive,scope)); }

		
			void				PrintToStream(ITextOutput::arg io, uint32 flags=0) const;
	
			status_t			Simplify();

			size_t				CountAlternatives() const;
			const				BMediaConstraintAlternative &AlternativeAt(size_t i) const;

			static bool Contains(const BMediaConstraintAlternative &a,
									const BMediaConstraintAlternative &b);
									
			BValue				AsValue() const;
		
private:
			BVector<BMediaConstraintAlternative> mAlternatives;
			bool mConjunctive;
			bool mAlwaysTrueIfEmpty;
		
			BMediaConstraintItem::simplify_result_t SimplifyOnce();

			void				Invert();
			void				Special();
};

ITextOutput::arg	operator<<(ITextOutput::arg io, const BMediaConstraint & constraint);

}; }; // namespace B::Media2

#endif //_MEDIA2_MEDIACONSTRAINT_H_
