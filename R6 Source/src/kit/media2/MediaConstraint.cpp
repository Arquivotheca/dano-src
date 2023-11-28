#include <media2/MediaConstraint.h>
#include <media2/MediaFormat.h>

#include <support2/StdIO.h>
#include <support2/Debug.h>

#include "convert_format.h"

namespace B {
namespace Private {

using namespace Support2;
using namespace Media2;

class MediaConstraintBuilder : public IValueOutput
{
	BMediaConstraint &	_c;
public:
	B_STANDARD_ATOM_TYPEDEFS(MediaConstraintBuilder)

					MediaConstraintBuilder(BMediaConstraint & c) : _c(c) {}
	status_t		Write(const BValue &out)
	{
		void * cookie = 0;
		BValue k, v;
		status_t err = out.GetNextItem(&cookie, &k, &v);
		if (err < B_OK) return err;
		_c.And(k, v);
		return B_OK;
	}
	status_t		End()
	{
		return B_OK;
	}
	
	protected:
		virtual	atom_ptr<IBinder>		AsBinderImpl()			{ return NULL; }
		virtual	atom_ptr<const IBinder>	AsBinderImpl() const	{ return NULL; }
};
} // Private

namespace Media2 {

using Private::MediaConstraintBuilder;

BMediaConstraint::BMediaConstraint(bool constant_boolean_value)
	: mConjunctive(false),
	  mAlwaysTrueIfEmpty(constant_boolean_value)
{
}

BMediaConstraint::BMediaConstraint(const BMediaConstraintItem &S)
	: mConjunctive(false),
	  mAlwaysTrueIfEmpty(false)
{
	BMediaConstraintAlternative C;	
	C.AddConstraintItem(S);

	mAlternatives.AddItem(C);
}


BMediaConstraint::BMediaConstraint(const BValue &key, const BValue &value,
									BMediaConstraintItem::constraint_scope_t scope)
	: mConjunctive(false),
	  mAlwaysTrueIfEmpty(false)
{
	BMediaConstraintItem S(key, BMediaConstraintItem::B_EQ, value,
							scope==BMediaConstraintItem::B_OPTIONAL);
	
	BMediaConstraintAlternative C;	
	C.AddConstraintItem(S);

	mAlternatives.AddItem(C);
}


BMediaConstraint::BMediaConstraint(const BValue &key,
							BMediaConstraintItem::relation_t rel,
							const BValue &value,
							BMediaConstraintItem::constraint_scope_t scope)
	: mConjunctive(false),
	  mAlwaysTrueIfEmpty(false)
{
	BMediaConstraintItem S(key,rel,value,scope==BMediaConstraintItem::B_OPTIONAL);
	
	BMediaConstraintAlternative C;	
	C.AddConstraintItem(S);

	mAlternatives.AddItem(C);
}

BMediaConstraint::BMediaConstraint(const BValue &key,
							const BValue &from, const BValue &to,
							bool from_inclusive, bool to_inclusive,
							BMediaConstraintItem::constraint_scope_t scope)
	: mConjunctive(false),
	  mAlwaysTrueIfEmpty(false)
{
	BMediaConstraintItem S1(key,BMediaConstraintItem::B_GE,from,
							scope==BMediaConstraintItem::B_OPTIONAL);

	Or(S1);

	if (from_inclusive)
	{
		BMediaConstraintItem S2(key,BMediaConstraintItem::B_EQ,from,
								scope==BMediaConstraintItem::B_OPTIONAL);
		Or(S2);
	}
	
	BMediaConstraint temp(key,BMediaConstraintItem::B_LT,to,scope);

	if (to_inclusive)
	{
		BMediaConstraintItem S4(key,BMediaConstraintItem::B_EQ,to,
								scope==BMediaConstraintItem::B_OPTIONAL);

		temp.Or(S4);
	}
	
	And(temp);
}

BMediaConstraint::BMediaConstraint(const BMediaFormat &format)
	: mConjunctive(false),
	  mAlwaysTrueIfEmpty(true)
{
	BValue key, value;
	void * cookie = 0;
	while (format.GetNextItem(&cookie, &key, &value) == B_OK)
	{
		And(key, BMediaConstraintItem::B_EQ, value);
	}
}

BMediaConstraint::BMediaConstraint(const media_format &format)
	: mConjunctive(false),
	  mAlwaysTrueIfEmpty(true)
{
	MediaConstraintBuilder::ptr cb = new MediaConstraintBuilder(*this);
	export_format_values(format, cb);
}

BMediaConstraint::BMediaConstraint(const BValue &archive)
	: mConjunctive(false),
	  mAlwaysTrueIfEmpty(true)
{
	if (archive["conjunctive"].GetBool(&mConjunctive) < B_OK) return;
	if (archive["true_if_empty"].GetBool(&mAlwaysTrueIfEmpty) < B_OK) return;
	BValue v;
	for (size_t n = 0; (v = archive[n]); n++)
	{
		mAlternatives.AddItem(BMediaConstraintAlternative(v));
	}
}

BValue 
BMediaConstraint::AsValue() const
{
	BValue v;
	v.Overlay("conjunctive", BValue::Bool(mConjunctive));
	v.Overlay("true_if_empty", BValue::Bool(mAlwaysTrueIfEmpty));
	const size_t count = mAlternatives.CountItems();
	for (size_t n = 0; n < count; n++) v.Overlay(BValue::Int32(n), mAlternatives[n].AsValue());
	return v;
}

BMediaConstraint &
BMediaConstraint::And(const BMediaConstraint &constraint)
{
	if (mAlternatives.CountItems()==0)
	{
		if (!mAlwaysTrueIfEmpty)
			return *this; // false AND ... <=> false
		else
			operator=(constraint);
		
		return *this;
	}
	else if (constraint.mAlternatives.CountItems()==0)
	{
		if (constraint.mAlwaysTrueIfEmpty)
			return *this; // false AND ... <=> false
		else
			operator=(constraint);
		
		return *this;
	}
		
	BVector<BMediaConstraintAlternative> newAlternatives;
	for (size_t i=0;i<mAlternatives.CountItems();++i)
	{
		for (size_t j=0;j<constraint.mAlternatives.CountItems();++j)
		{
			BMediaConstraintAlternative C=mAlternatives.ItemAt(i);
			C.AppendConstraintAlternative(constraint.mAlternatives.ItemAt(j));

			newAlternatives.AddItem(C);
		}
	}
	mAlternatives=newAlternatives;
	
	Simplify();
	
	return *this;
}

BMediaConstraint & 
BMediaConstraint::Or(const BMediaConstraint &constraint)
{
	if (mAlternatives.CountItems()==0)
	{
		if (mAlwaysTrueIfEmpty)
			return *this; // true OR ... <=> true
		else
			operator=(constraint);
		
		return *this;
	}
	else if (constraint.mAlternatives.CountItems()==0)
	{
		if (!mAlwaysTrueIfEmpty)
			return *this; // true OR ... <=> true
		else
			operator=(constraint);
		
		return *this;
	}

	mAlternatives.AddVector(constraint.mAlternatives);
	
	Simplify();
	
	return *this;
}

void 
BMediaConstraint::PrintToStream (ITextOutput::arg io, uint32) const
{
	const int32 n=mAlternatives.CountItems();
	
	if (n==0)
	{
		io<<(mAlwaysTrueIfEmpty ? "true" : "false")<<endl;
	}
	else
	{
		for (int32 i=0;i<n;++i)
		{
			mAlternatives.ItemAt(i).PrintToStream(io);
			
			io<<endl;
			
			if (i+1<n)
				io<<(mConjunctive ? "&&" : "||");
		}
	}
}

BMediaConstraintItem::simplify_result_t 
BMediaConstraint::SimplifyOnce()
{
	for (size_t i=0;i<mAlternatives.CountItems();++i)
	{
		BMediaConstraintItem::simplify_result_t result=mAlternatives.EditItemAt(i).Simplify();

		if (mConjunctive)
		{
			if (result==BMediaConstraintItem::B_FALSE)
			{
				mAlternatives.MakeEmpty();
				return result;
			}
			else if (result==BMediaConstraintItem::B_TRUE)
			{
				mAlternatives.RemoveItemsAt(i);
				--i;
			}
		}
		else
		{
			if (result==BMediaConstraintItem::B_TRUE)
			{
				mAlternatives.MakeEmpty();
				return result;
			}
			else if (result==BMediaConstraintItem::B_FALSE)
			{
				mAlternatives.RemoveItemsAt(i);
				--i;
			}
		}
	}
	
	Special();
	
	return mAlternatives.CountItems()==0 ? (mConjunctive ? BMediaConstraintItem::B_TRUE
														 : BMediaConstraintItem::B_FALSE)
										 : BMediaConstraintItem::B_NONE;
}

// +++ a more evocative error code than B_ERROR would be handy
status_t 
BMediaConstraint::Simplify()
{
	if (mAlternatives.CountItems()==0)
	{
		return mAlwaysTrueIfEmpty ? B_OK : B_ERROR;
	}
	
	BMediaConstraintItem::simplify_result_t result=SimplifyOnce();

	if (result==BMediaConstraintItem::B_TRUE)
	{
		mAlwaysTrueIfEmpty=true;
		return B_OK;
	}
	else if (result==BMediaConstraintItem::B_FALSE)
	{
		mAlwaysTrueIfEmpty=false;
		return B_ERROR;
	}	
		
	Invert();

	result=SimplifyOnce();

	Invert();

	if (result==BMediaConstraintItem::B_TRUE)
	{
		mAlwaysTrueIfEmpty=false;
		return B_OK;
	}
	else if (result==BMediaConstraintItem::B_FALSE)
	{
		mAlwaysTrueIfEmpty=true;
		return B_ERROR;
	}
	
	result=SimplifyOnce();

	if (result==BMediaConstraintItem::B_TRUE)
	{
		mAlwaysTrueIfEmpty=true;
		return B_OK;
	}
	else if (result==BMediaConstraintItem::B_FALSE)
	{
		mAlwaysTrueIfEmpty=false;
		return B_ERROR;
	}

	return B_OK;
}

void 
BMediaConstraint::Invert()
{
	if (mAlternatives.CountItems()==0)
		return;
	
	size_t n=mAlternatives.CountItems();

	BMediaConstraint C;
	C.mConjunctive=!mConjunctive;
	
	const BMediaConstraintAlternative &A=mAlternatives.ItemAt(n-1);
	
	for (ssize_t i=A.mConstraintItems.CountItems()-1;i>=0;--i)
	{
		const BMediaConstraintItem &I=A.mConstraintItems[i];
		
		BMediaConstraintAlternative &B=C.mAlternatives.EditItemAt(C.mAlternatives.AddItem());
		
		B.AddConstraintItem(I);
	}
	
	mAlternatives.RemoveItemsAt(n-1);
	--n;

	while (n>0)
	{
		// remove last alternative
		
		BMediaConstraintAlternative A=mAlternatives.ItemAt(n-1);
		mAlternatives.RemoveItemsAt(n-1);
		--n;

		BMediaConstraint newC;
		newC.mConjunctive=C.mConjunctive;
		
		for (ssize_t i=C.mAlternatives.CountItems()-1;i>=0;--i)
		{
			for (ssize_t j=A.mConstraintItems.CountItems()-1;j>=0;--j)
			{
				ssize_t new_index;
				BMediaConstraintAlternative &B=newC.mAlternatives.EditItemAt(new_index=newC.mAlternatives.AddItem());
				
				B=C.mAlternatives[i];
				B.AddConstraintItem(A.mConstraintItems[j]);

				B.mConjunctive=!newC.mConjunctive;
				
				BMediaConstraintItem::simplify_result_t result=B.Simplify();
				
				if (result==BMediaConstraintItem::B_TRUE
					|| result==BMediaConstraintItem::B_FALSE)
				{
					newC.mAlternatives.RemoveItemsAt(new_index);
				}
			}
		}
		
		C=newC;
	}
	
	(void)operator=(C);
}

bool
BMediaConstraint::Contains (const BMediaConstraintAlternative &a, const BMediaConstraintAlternative &b)
{
	for (size_t i=0;i<b.mConstraintItems.CountItems();++i)
	{
		bool found=false;
		for (size_t j=0;!found && j<a.mConstraintItems.CountItems();++j)
		{
			ASSERT(j<a.mConstraintItems.CountItems());
			ASSERT(uint32(&a.mConstraintItems[j])>=0x80000000);
			ASSERT(i<b.mConstraintItems.CountItems());
			ASSERT(uint32(&b.mConstraintItems[i])>=0x80000000);
			
			if (a.mConstraintItems[j]==b.mConstraintItems[i])
				found=true;
		}
		
		if (!found)
			return false;
	}
	
	return true;
}

void 
BMediaConstraint::Special()
{
	BVector<bool> alternativeEliminated;
	for (size_t i=0;i<mAlternatives.CountItems();++i)
		alternativeEliminated.EditItemAt(alternativeEliminated.AddItem())=false;
		
	for (size_t i=0;i<mAlternatives.CountItems();++i)
	{
		if (!alternativeEliminated[i])
		{
			const BMediaConstraintAlternative &alternative=mAlternatives.ItemAt(i);
			
			for (size_t j=0;j<mAlternatives.CountItems();++j)
			{
				if (j==i || alternativeEliminated[j])
					continue;
			
				const BMediaConstraintAlternative &superset=mAlternatives.ItemAt(j);
	
				if (Contains(superset,alternative))
					alternativeEliminated.EditItemAt(j)=true;
			}
		}
	}
	
	for (ssize_t i=mAlternatives.CountItems()-1;i>=0;--i)
	{
		if (alternativeEliminated[i])
			mAlternatives.RemoveItemsAt(i);
	}
}

size_t 
BMediaConstraint::CountAlternatives() const
{
	return mAlternatives.CountItems();
}

const BMediaConstraintAlternative &
BMediaConstraint::AlternativeAt(size_t i) const
{
	return mAlternatives.ItemAt(i);
}

ITextOutput::arg 
operator<<(ITextOutput::arg io, const BMediaConstraint &constraint)
{
	constraint.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

}; }; // namespace B::Media2
