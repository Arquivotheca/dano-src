/***************************************************************************
//
//	File:			media2/MediaConstraintItem.h
//
//	Description:	Relates a key to a value in one of these ways:
//					"equal to"
//					"less than"
//					"multiple of"
//					"power of"
//					or the inverse of any of these.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIACONSTRAINTITEM_H_
#define _MEDIA2_MEDIACONSTRAINTITEM_H_

#include <support2/String.h>
#include <support2/Value.h>

namespace B {
namespace Media2 {

using namespace Support2;

class BMediaConstraintItem
{	
public:		
	enum relation_t
	{
		B_EQ				= 0,
		B_ONE_OF			= B_EQ,
		B_LT,
		B_MULTIPLE_OF,
		B_POWER_OF,
		B_NAME_DEPENDENT,				// internal use only !!!
		B_INVERT			= 0x100,
		B_NE				= B_INVERT,
		B_NOT_ANY_OF		= B_INVERT,
		B_GE,
		B_NOT_MULTIPLE_OF,
		B_NOT_POWER_OF
	};

	enum constraint_scope_t
	{
		B_MANDATORY = 0,
		B_OPTIONAL
	};
	
	enum simplify_result_t
	{
		B_KEEP_FIRST,
		B_KEEP_SECOND,
		B_KEEP_BOTH,
		B_FALSE,
		B_TRUE,
		B_NONE
	};

								BMediaConstraintItem();
		
								BMediaConstraintItem(
									const BValue &key,
									relation_t rel,
									const BValue &value,
									bool optional);
									
								BMediaConstraintItem(
									const BValue & archive);

			void				PrintToStream(ITextOutput::arg io) const;
		
			bool				operator==(const BMediaConstraintItem &S) const;
				
			BValue				Key() const;
			relation_t			Relation() const;
			BValue 				Value() const;
			
			BValue				AsValue() const;
			inline  bool				IsOptional() const { return mOptional; }
		
private:
	friend class BMediaConstraintAlternative;

			bool				mIsValid;
			BValue				mKey;
			relation_t			mRel;
			BValue				mValue;
			bool				mOptional;
			
	inline	relation_t			BaseRelation() const { return relation_t(mRel & ~B_INVERT); }
	inline	bool				IsInverted() const { return mRel & B_INVERT; }
	
		static simplify_result_t Simplify (bool conjunctive,
											BMediaConstraintItem &S1,
											BMediaConstraintItem &S2);

		static simplify_result_t SimplifyLTONE_OF (bool conjunctive,
											BMediaConstraintItem &S1,
											BMediaConstraintItem &S2);

		static simplify_result_t SimplifyLTLT (bool conjunctive,
											BMediaConstraintItem &S1,
											BMediaConstraintItem &S2);

		static simplify_result_t SimplifyONE_OFMULTIPLE_OF (bool conjunctive,
											BMediaConstraintItem &S1,
											BMediaConstraintItem &S2);

		static simplify_result_t SimplifyMULTIPLE_OFMULTIPLE_OF (bool conjunctive,
											BMediaConstraintItem &S1,
											BMediaConstraintItem &S2);

		static simplify_result_t SimplifyONE_OFONE_OF (bool conjunctive,
											BMediaConstraintItem &S1,
											BMediaConstraintItem &S2);

		static BValue &RemoveCriteria(BValue &set,
										bool (*meets_criteria)(const BValue &value, const BValue &param),
										const BValue &param);

		static bool GreaterOrEqualThan(const BValue &value, const BValue &param);
		static bool LessThan(const BValue &value, const BValue &param);

		static bool IsNotMultipleOf(const BValue &value, const BValue &param);
		static bool IsMultipleOf(const BValue &value, const BValue &param);

		static int64 GetValueAsInt64(const BValue &value);
		static BValue Intersection(const BValue &n1, const BValue &n2);
};

}; }; // namespace B::Media2

#endif // _MEDIA2_MEDIACONSTRAINTITEM_H_
