#include <media2/MediaConstraintItem.h>

#include <support2/Debug.h>
#include <OS.h>

namespace B {
namespace Media2 {

BMediaConstraintItem::BMediaConstraintItem()
	: mIsValid(false)
{
}

BMediaConstraintItem::BMediaConstraintItem(const BValue &key,
												relation_t rel,
												const BValue &value,
												bool optional)
	: mIsValid(true),
	  mKey(key),
	  mRel(rel),
	  mValue(value),
	  mOptional(optional)
{
}


BMediaConstraintItem::BMediaConstraintItem(const BValue &archive)
	: mIsValid(true),
	  mKey(archive["key"]),
	  mRel(static_cast<relation_t>(archive["rel"].AsInteger())),
	  mValue(archive["val"]),
	  mOptional(archive["optional"].AsBool())
{
}

BValue 
BMediaConstraintItem::AsValue() const
{
	return BValue().
		Overlay("key", mKey).
		Overlay("rel", BValue::Int32(mRel)).
		Overlay("val", mValue).
		Overlay("optional", BValue::Bool(mOptional));
}

void 
BMediaConstraintItem::PrintToStream (ITextOutput::arg io) const
{
	if (BaseRelation()==B_NAME_DEPENDENT)
	{
		io<<"[ ";
		io<<(mValue.AsBool() ? "true" : "false");
		io<<" if "<<mKey<<" specified ]";
		return;
	}
	
	if (mOptional)
		io<<"[ ";
		
	io<<mKey;

	if (!IsInverted())
	{
		switch (BaseRelation())
		{
			case B_LT:
			{
				io<<"<"<<mValue;			
				break;
			}
			
			case B_MULTIPLE_OF:
			{
				io<<"%"<<mValue<<" == 0";			
				break;
			}
	
			case B_POWER_OF:
			{
				io<<" power_of "<<mValue;
				break;
			}
			
			case B_ONE_OF:
			{
				io<<" is_one_of "<<mValue;
				break;
			}
			
			default:
				TRESPASS();
				break;
		}
	}
	else
	{
		switch (BaseRelation())
		{
			case B_LT:
			{
				io<<">="<<mValue;			
				break;
			}
			
			case B_MULTIPLE_OF:
			{
				io<<"%"<<mValue<<" != 0";
				break;
			}
	
			case B_POWER_OF:
			{
				io<<" not power_of "<<mValue;
				break;
			}
			
			case B_ONE_OF:
			{
				io<<" not_any_of "<<mValue;
				break;
			}
			
			default:
				TRESPASS();
				break;
		}
	}	

	if (mOptional)
		io<<" ]";
}

bool 
BMediaConstraintItem::operator==(const BMediaConstraintItem &S) const
{
	return mIsValid && S.mIsValid
			&& mKey==S.mKey && mRel==S.mRel && mValue == S.mValue && mOptional==S.mOptional;
}

BValue
BMediaConstraintItem::Key() const
{
	return mKey;
}

BMediaConstraintItem::relation_t 
BMediaConstraintItem::Relation() const
{
	return mRel;
}

BValue
BMediaConstraintItem::Value() const
{
	return mValue;
}

BMediaConstraintItem::simplify_result_t 
BMediaConstraintItem::Simplify(bool conjunctive,
									BMediaConstraintItem &S1,
									BMediaConstraintItem &S2)
{
	if (S1.mKey!=S2.mKey)
		return B_KEEP_BOTH;

	bool result_is_optional;
	if (S1.IsOptional() && S2.IsOptional())
		result_is_optional=true;
	else
		result_is_optional=false;
		
	BMediaConstraintItem::simplify_result_t result;
	
	if (S1.BaseRelation()==B_LT && S2.BaseRelation()==B_ONE_OF)
		result=SimplifyLTONE_OF(conjunctive,S1,S2);
	else if (S1.BaseRelation()==B_LT && S2.BaseRelation()==B_LT)
		result=SimplifyLTLT(conjunctive,S1,S2);
	else if (S1.BaseRelation()==B_ONE_OF && S2.BaseRelation()==B_ONE_OF)
		result=SimplifyONE_OFONE_OF(conjunctive,S1,S2);
	else if (S1.BaseRelation()==B_ONE_OF && S2.BaseRelation()==B_MULTIPLE_OF)
		result=SimplifyONE_OFMULTIPLE_OF(conjunctive,S1,S2);
	else if (S1.BaseRelation()==B_MULTIPLE_OF && S2.BaseRelation()==B_MULTIPLE_OF)
		result=SimplifyMULTIPLE_OFMULTIPLE_OF(conjunctive,S1,S2);
	else if (S1.BaseRelation()==B_NAME_DEPENDENT)
	{
		// S1.optional is always true
		
		bool constant_bool=S1.mValue.AsBool();
		
		if (conjunctive) // [constant] && ...
		{
			if (constant_bool) // [true] && ...
				result=B_KEEP_SECOND;
			else				// [false] && ...
				result=B_FALSE;
		}
		else			// [constant] || ...
		{
			if (constant_bool) // [true] || ...
				result=B_TRUE;
			else				// [false] || ...
				result=B_KEEP_SECOND;
		}
	}
	else
		result=B_KEEP_BOTH;	

	switch (result)
	{
		case B_KEEP_FIRST:
			S1.mOptional=result_is_optional;
			break;
			
		case B_KEEP_SECOND:
			S2.mOptional=result_is_optional;
			break;
			
		case B_KEEP_BOTH:
			S1.mOptional=result_is_optional;
			S2.mOptional=result_is_optional;
			break;
		
		case B_FALSE:
		case B_TRUE:
		{
			if (result_is_optional)
			{
				S1.mOptional=true;
				S1.mRel=B_NAME_DEPENDENT;
				S1.mValue=BValue::Bool(result==B_FALSE ? false : true);
				
				result=B_KEEP_FIRST;
			}
			
			break;
		}
		
		case B_NONE:
		{
			// cannot happen
			
			TRESPASS();
			break;
		}
	}
				
	return result;
}

BValue &
BMediaConstraintItem::RemoveCriteria (BValue &set,
										bool (*meets_criteria)(const BValue &value, const BValue &param),
										const BValue &param)
{
	BValue matches;
	
	void *cookie=NULL;
	BValue key,value;
	while (set.GetNextItem(&cookie,&key,&value)>=B_OK)
	{
		ASSERT(key.IsNull());
		
		if ((*meets_criteria)(value,param))
			matches.Overlay(key,value);
	}
	
	set.Remove(matches);
	
	return set;
}

bool
BMediaConstraintItem::GreaterOrEqualThan (const BValue &value, const BValue &param)
{
	return value>=param;
}

bool
BMediaConstraintItem::LessThan (const BValue &value, const BValue &param)
{
	return value<param;
}

int64
BMediaConstraintItem::GetValueAsInt64 (const BValue &value)
{
	switch (value.Type())
	{
		case B_INT8_TYPE:
		{
			int8 val;
			value.GetInt8(&val);
			return val;
		}

		case B_INT16_TYPE:
		{
			int16 val;
			value.GetInt16(&val);
			return val;
		}

		case B_INT32_TYPE:
		{
			int32 val;
			value.GetInt32(&val);
			return val;
		}

		case B_INT64_TYPE:
		{
			int64 val;
			value.GetInt64(&val);
			return val;
		}
		
		case B_FLOAT_TYPE:
		{
			float val;
			value.GetFloat(&val);
			return int64(val);		// warning: truncation
		}

		case B_DOUBLE_TYPE:
		{
			double val;
			value.GetDouble(&val);
			return int64(val);		// warning: truncation
		}
		
		default:
			TRESPASS();
			return 0;
			break;
	}
}

bool 
BMediaConstraintItem::IsNotMultipleOf(const BValue &value, const BValue &param)
{
	int64 denom=GetValueAsInt64(param);
	ASSERT(denom!=0);
	
	return (GetValueAsInt64(value) % denom)!=0;
}

bool 
BMediaConstraintItem::IsMultipleOf(const BValue &value, const BValue &param)
{
	int64 denom=GetValueAsInt64(param);
	ASSERT(denom!=0);

	return (GetValueAsInt64(value) % denom)==0;
}

BMediaConstraintItem::simplify_result_t 
BMediaConstraintItem::SimplifyLTONE_OF (bool conjunctive,
									BMediaConstraintItem &S1,
									BMediaConstraintItem &S2)
{
	if (conjunctive)
	{
		if (!S1.IsInverted() && !S2.IsInverted())
		{
			// a<n1 && a in n2 <=> {n2<n1} empty ? false : a in {n2<n1}
			
			RemoveCriteria(S2.mValue,GreaterOrEqualThan,S1.mValue);

			return !S2.mValue.IsDefined() ? B_FALSE : B_KEEP_SECOND;			
		}
		else if (S1.IsInverted() && !S2.IsInverted())
		{
			// a>=n1 && a in n2	<=>	{n2>=n1} empty ? false : a in {n2>=n1}

			RemoveCriteria(S2.mValue,LessThan,S1.mValue);
						
			return !S2.mValue.IsDefined() ? B_FALSE : B_KEEP_SECOND;			
		}
		else if (!S1.IsInverted() && S2.IsInverted())
		{
			// a<n1 && !(a in n2)	<=>		{n2>=n1} empty ? a<n1 : a<n1 && !(a in {n2>=n1})

			RemoveCriteria(S2.mValue,LessThan,S1.mValue);

			return !S2.mValue.IsDefined() ? B_KEEP_FIRST : B_KEEP_BOTH;			
		}
		else
		{
			// a>=n1 && !(a in n2)	<=>		{n2<n1} empty ? a>=n1 : a>=n1 && !(a in {n2<n1})
			
			RemoveCriteria(S2.mValue,GreaterOrEqualThan,S1.mValue);

			return !S2.mValue.IsDefined() ? B_KEEP_FIRST : B_KEEP_BOTH;			
		}
	}
	else
	{
		if (!S1.IsInverted() && !S2.IsInverted())
		{
			// a<n1 || a in n2		<=>		
			
			RemoveCriteria(S2.mValue,LessThan,S1.mValue);
			
			return !S2.mValue.IsDefined() ? B_KEEP_FIRST : B_KEEP_BOTH;
		}
		else if (S1.IsInverted() && !S2.IsInverted())
		{
			// !(a<n1) || a in n2	<=>
			
			RemoveCriteria(S2.mValue,GreaterOrEqualThan,S1.mValue);
			
			return !S2.mValue.IsDefined() ? B_KEEP_FIRST : B_KEEP_BOTH;
		}
		else if (!S1.IsInverted() && S2.IsInverted())
		{
			// a<n1 || !(a in n2)	<=>
			
			RemoveCriteria(S2.mValue,LessThan,S1.mValue);
			
			return !S2.mValue.IsDefined() ? B_TRUE : B_KEEP_SECOND;
		}
		else
		{
			// !(a<n1) || !(a in n2)
			// <=>		a>=n1 || !(a in n2)	<=> 

			RemoveCriteria(S2.mValue,GreaterOrEqualThan,S1.mValue);
			
			return !S2.mValue.IsDefined() ? B_TRUE : B_KEEP_SECOND;						
		}
	}
}

BMediaConstraintItem::simplify_result_t 
BMediaConstraintItem::SimplifyLTLT (bool conjunctive,
									BMediaConstraintItem &S1,
									BMediaConstraintItem &S2)
{
	if (conjunctive)
	{
		if (!S1.IsInverted() && !S2.IsInverted())
		{
			// a<n1 && a<n2 		<=>		a<min(n1,n2)
			
			S1.mValue=min_c(S1.mValue,S2.mValue);
			
			return B_KEEP_FIRST;
		}
		else if (S1.IsInverted() && !S2.IsInverted())
		{
			// !(a<n1) && a<n2		<=>		a>=n1 && a<n2	<=>	a in [n1,n2[
			// 						<=>	n1<n2 ? same : false
	
			if (S1.mValue<S2.mValue)
				return B_KEEP_BOTH;
			else
				return B_FALSE;								
		}
		else if (!S1.IsInverted() && S2.IsInverted())
		{
			// a<n1 && !(a<n2)		<=>		a<n1 && a>=n2	<=>	a in [n2,n1[
			//						<=> n2<n1 ? same : false
	
			if (S2.mValue<S1.mValue)
				return B_KEEP_BOTH;
			else
				return B_FALSE;
		}
		else
		{
			// !(a<n1) && !(a<n2)	<=>		a>=n1 && a>=n2	<=>	!(a<max(n1,n2))
			
			S1.mValue=max_c(S1.mValue,S2.mValue);
			
			return B_KEEP_FIRST;
		}
	}
	else
	{
		if (!S1.IsInverted() && !S2.IsInverted())
		{
			// a<n1 || a<n2 		<=>		!(!(a<n1) && !(a<n2))	<=>	a<max(n1,n2)
			
			S1.mValue=max_c(S1.mValue,S2.mValue);
			
			return B_KEEP_FIRST;
		}
		else if (S1.IsInverted() && !S2.IsInverted())
		{
			// !(a<n1) || a<n2		<=>		!(a<n1 && !(a<n2))
			// <=>	n2<n1 ? same : true
	
			if (S2.mValue<S1.mValue)
				return B_KEEP_BOTH;
			else
				return B_TRUE;								
		}
		else if (!S1.IsInverted() && S2.IsInverted())
		{
			// a<n1 || !(a<n2)		<=>		!(!(a<n1) && a<n2)
			// <=>	n1<n2 ? same : true
	
			if (S1.mValue<S2.mValue)
				return B_KEEP_BOTH;
			else
				return B_TRUE;
		}
		else
		{
			// !(a<n1) || !(a<n2)	<=>		!(a<n1 && a<n2)
			// <=> !(a<min(n1,n2))
			
			S1.mValue=min_c(S1.mValue,S2.mValue);
			
			return B_KEEP_FIRST;
		}
	}
}

BMediaConstraintItem::simplify_result_t 
BMediaConstraintItem::SimplifyONE_OFMULTIPLE_OF (bool conjunctive,
									BMediaConstraintItem &S1,
									BMediaConstraintItem &S2)
{
	if (conjunctive)
	{
		if (!S1.IsInverted() && !S2.IsInverted())
		{
			// a in n1 && a multiple_of n2	<=>	
			
			RemoveCriteria(S1.mValue,IsNotMultipleOf,S2.mValue);

			return !S1.mValue.IsDefined() ? B_FALSE : B_KEEP_FIRST;			
		}
		else if (S1.IsInverted() && !S2.IsInverted())
		{
			// !(a in n1) && a multiple_of n2
			//	<=>	
			
			RemoveCriteria(S1.mValue,IsNotMultipleOf,S2.mValue);

			return !S1.mValue.IsDefined() ? B_KEEP_SECOND : B_KEEP_BOTH;			
		}
		else if (!S1.IsInverted() && S2.IsInverted())
		{
			// a in n1 && !(a multiple of n2)	<=> 
			
			RemoveCriteria(S1.mValue,IsMultipleOf,S2.mValue);

			return !S1.mValue.IsDefined() ? B_FALSE : B_KEEP_FIRST;			
		}
		else
		{
			// !(a in n1) && !(a multiple_of n2)
			//	<=>	
			
			RemoveCriteria(S1.mValue,IsMultipleOf,S2.mValue);

			return !S1.mValue.IsDefined() ? B_KEEP_SECOND : B_KEEP_BOTH;			
		}
	}
	else
	{
		if (!S1.IsInverted() && !S2.IsInverted())
		{
			// a in n1 || a multiple_of n2		<=>
			
			RemoveCriteria(S1.mValue,IsMultipleOf,S2.mValue);

			return !S1.mValue.IsDefined() ? B_KEEP_SECOND : B_KEEP_BOTH;			
		}
		else if (S1.IsInverted() && !S2.IsInverted())
		{
			// !(a in n1) || a multiple_of n2	<=>
			
			RemoveCriteria(S1.mValue,IsMultipleOf,S2.mValue);

			return !S1.mValue.IsDefined() ? B_KEEP_SECOND : B_KEEP_BOTH;			
		}
		else if (!S1.IsInverted() && S2.IsInverted())
		{
			// a in n1 || !(a multiple of n2)	<=>
			
			RemoveCriteria(S1.mValue,IsNotMultipleOf,S2.mValue);

			return !S1.mValue.IsDefined() ? B_KEEP_SECOND : B_KEEP_BOTH;			
		}
		else
		{
			// !(a in n1) || !(a multiple_of n2)	<=>
			
			RemoveCriteria(S1.mValue,IsNotMultipleOf,S2.mValue);

			return !S1.mValue.IsDefined() ? B_KEEP_SECOND : B_KEEP_BOTH;			
		}
	}
}

BMediaConstraintItem::simplify_result_t 
BMediaConstraintItem::SimplifyMULTIPLE_OFMULTIPLE_OF (bool conjunctive,
									BMediaConstraintItem &S1,
									BMediaConstraintItem &S2)
{
	if (conjunctive)
	{
		if (!S1.IsInverted() && !S2.IsInverted())
		{
			// a multiple_of n1 && a multiple_of n2 <=> a multiple_of lcm(n1,n2)
			
			int64 S1val=GetValueAsInt64(S1.mValue);
			int64 least_common_multiple=S1val;
			int64 S2val=GetValueAsInt64(S2.mValue);
			
			while (least_common_multiple<S1val*S2val)
			{
				if ((least_common_multiple % S2val)==0)
					break;
				
				least_common_multiple+=S1val;
			}
			
			S1.mValue=BValue::Int64(least_common_multiple);
			
			return B_KEEP_FIRST;
		}
		else if (S1.IsInverted() && !S2.IsInverted())
		{
			// !(a multiple_of n1) && a multiple_of n2
			// <=> (n2 % n1)==0 ? false : same
			
			return IsMultipleOf(S2.mValue,S1.mValue) ? B_FALSE : B_KEEP_BOTH;				
		}
		else if (!S1.IsInverted() && S2.IsInverted())
		{
			// a multiple_of n1 && !(a multiple_of n2)
			// <=> (n1 % n2)==0 ? false : same
			
			return IsMultipleOf(S1.mValue,S2.mValue) ? B_FALSE : B_KEEP_BOTH;				
		}
		else
		{
			// !(a multiple_of n1) && !(a multiple_of n2)
			// <=> !(a multiple_of n1 || a multiple_of n2)
			// <=> (n1 % n2)==0 ? !(a multiple_of n2) :
			// 	   (n2 % n1)==0 ? !(a multiple_of n1)
			//					: same

			if (IsMultipleOf(S1.mValue,S2.mValue))
				return B_KEEP_SECOND;
			else if (IsMultipleOf(S2.mValue,S1.mValue))
				return B_KEEP_FIRST;
			else
				return B_KEEP_BOTH;
		}
	}
	else
	{
		if (!S1.IsInverted() && !S2.IsInverted())
		{
			// (a multiple_of n1) || (a multiple_of n2)
			// <=> !(!a multiple_of n1 && !a multiple_of n2)
			// <=> (n1 % n2)==0 ? a multiple_of n2 :
			// 	   (n2 % n1)==0 ? a multiple_of n1
			//					: same

			if (IsMultipleOf(S1.mValue,S2.mValue))
				return B_KEEP_SECOND;
			else if (IsMultipleOf(S2.mValue,S1.mValue))
				return B_KEEP_FIRST;
			else
				return B_KEEP_BOTH;
		}
		else if (S1.IsInverted() && !S2.IsInverted())
		{
			// !(a multiple_of n1) || a multiple_of n2 <=> (n1 % n2)==0 ? true : same
			
			if (IsMultipleOf(S1.mValue,S2.mValue))
				return B_TRUE;
			else
				return B_KEEP_BOTH;
		}
		else if (!S1.IsInverted() && S2.IsInverted())
		{
			// a multiple_of n1 || !(a multiple_of n2) <=> (n2 % n1)==0 ? true : same

			if (IsMultipleOf(S2.mValue,S1.mValue))
				return B_TRUE;
			else
				return B_KEEP_BOTH;
		}
		else
		{
			// !(a multiple_of n1) || !(a multiple_of n2) <=> !(a multiple_of lcm(n1,n2))

			int64 S1val=GetValueAsInt64(S1.mValue);
			int64 least_common_multiple=S1val;
			int64 S2val=GetValueAsInt64(S2.mValue);
			
			while (least_common_multiple<S1val*S2val)
			{
				if ((least_common_multiple % S2val)==0)
					break;
				
				least_common_multiple+=S1val;
			}
			
			S1.mValue=BValue::Int64(least_common_multiple);
			
			return B_KEEP_FIRST;			
		}
	}
}

BValue
BMediaConstraintItem::Intersection (const BValue &n1, const BValue &n2)
{
	BValue result;
	
	BValue key,value;
	void *cookie=NULL;
	while (n1.GetNextItem(&cookie,&key,&value)>=B_OK)
	{
		ASSERT(key.IsNull());
		
		if (n2.HasItem(value_ref(key),value_ref(value)))
			result.Overlay(key,value);
	}
	
	return result;
}

BMediaConstraintItem::simplify_result_t 
BMediaConstraintItem::SimplifyONE_OFONE_OF (bool conjunctive,
									BMediaConstraintItem &S1,
									BMediaConstraintItem &S2)
{
	if (conjunctive)
	{
		if (!S1.IsInverted() && !S2.IsInverted())
		{
			// a in n1 && a in n2		<=>		a in Intersection(n1,n2)

			S1.mValue=Intersection(S1.mValue,S2.mValue);
			
			if (!S1.mValue.IsDefined())
				return B_FALSE;
			else
				return B_KEEP_FIRST;			
		}
		else if (S1.IsInverted() && !S2.IsInverted())
		{
			// !(a in n1) && a in n2	<=>		a in n2\n1
			
			S2.mValue.Remove(S1.mValue);
			
			if (!S2.mValue.IsDefined())
				return B_FALSE;
			else
				return B_KEEP_SECOND;			
		}
		else if (!S1.IsInverted() && S2.IsInverted())
		{
			// a in n1 && !(a in n2)	<=>		a in n1\n2
			
			S1.mValue.Remove(S2.mValue);
			
			if (!S1.mValue.IsDefined())
				return B_FALSE;
			else
				return B_KEEP_FIRST;			
		}
		else
		{
			// !(a in n1) && !(a in n2)	<=>	not(a in Union(n1,n2))
			
			S1.mValue.Inherit(S2.mValue);
			
			if (!S1.mValue.IsDefined())
				return B_FALSE;
			else
				return B_KEEP_FIRST;			
		}
	}
	else
	{
		if (!S1.IsInverted() && !S2.IsInverted())
		{
			// a in n1 || a in n2		<=>		a in Union(n1,n2)
			
			S1.mValue.Inherit(S2.mValue);
			
			return B_KEEP_FIRST;			
		}
		else if (S1.IsInverted() && !S2.IsInverted())
		{
			// !(a in n1) || a in n2	<=>		not(a in n1\n2) || a in n2\n1
			
			BValue temp1=S1.mValue.RemoveCopy(S2.mValue);
			BValue temp2=S2.mValue.RemoveCopy(S1.mValue);
			
			S1.mValue=temp1;
			S2.mValue=temp2;
			
			if (!S1.mValue.IsDefined())
				return !S2.mValue.IsDefined() ? B_TRUE : B_KEEP_SECOND;
			else if (!S2.mValue.IsDefined())
				return B_KEEP_FIRST;
			else
				return B_KEEP_BOTH;
		}
		else if (!S1.IsInverted() && S2.IsInverted())
		{
			// a in n1 || !(a in n2)	<=>		not(a in n2\n1) || a in n1\n2
			
			BValue temp1=S2.mValue.RemoveCopy(S1.mValue);
			BValue temp2=S1.mValue.RemoveCopy(S2.mValue);
			
			S1.mValue=temp1;
			S2.mValue=temp2;
			
			if (!S1.mValue.IsDefined())
				return !S2.mValue.IsDefined() ? B_TRUE : B_KEEP_SECOND;
			else if (!S2.mValue.IsDefined())
				return B_KEEP_FIRST;
			else
				return B_KEEP_BOTH;
		}
		else
		{
			// !(a in n1) || !(a in n2)	<=>	not(a in Intersection(n1,n2))
			
			S1.mValue=Intersection(S1.mValue,S2.mValue);
			
			if (!S1.mValue.IsDefined())
				return B_TRUE;
			else
				return B_KEEP_FIRST;			
		}
	}
}

}; }; // namespace B::Media2
