#include <media2/MediaConstraintAlternative.h>
#include <media2/MediaFormat.h>

#include <support2/Debug.h>

namespace B {

namespace Media2 {

BMediaConstraintAlternative::BMediaConstraintAlternative()
	: mConjunctive(true)
{
}

BMediaConstraintAlternative::BMediaConstraintAlternative(const BValue &archive)
{
	status_t err = archive["conjunctive"].GetBool(&mConjunctive);
	if (err < B_OK) return;
	BValue v;
	for (size_t n = 0; (v = archive[n]); n++)
	{
		mConstraintItems.AddItem(BMediaConstraintItem(v));
	}
}

BValue 
BMediaConstraintAlternative::AsValue() const
{
	BValue v("conjunctive", BValue::Bool(mConjunctive));
	const size_t count = mConstraintItems.CountItems();
	for (size_t n = 0; n < count; n++) v.Overlay(BValue::Int32(n), mConstraintItems[n].AsValue());
	return v;
}

void 
BMediaConstraintAlternative::AddConstraintItem(const BMediaConstraintItem &S)
{
	mConstraintItems.AddItem(S);
}

void 
BMediaConstraintAlternative::AppendConstraintAlternative(const BMediaConstraintAlternative &C)
{
	mConstraintItems.AddVector(C.mConstraintItems);
}

void 
BMediaConstraintAlternative::PrintToStream(ITextOutput::arg io) const
{
	io<<"(";

	const int32 n=mConstraintItems.CountItems();
	
	for (int32 i=0;i<n;++i)
	{
		mConstraintItems.ItemAt(i).PrintToStream(io);
		
		if (i+1<n)
			io<<(mConjunctive ? "&&" : "||");
	}

	io<<")";
}

BMediaConstraintItem::simplify_result_t 
BMediaConstraintAlternative::Simplify()
{
	BMediaConstraintItem replace_with;
	
	bool changed;
	
	do
	{
		changed=false;
		
		for (size_t i=0;!changed && i<mConstraintItems.CountItems();++i)
		{
			for (size_t j=0;!changed && j<mConstraintItems.CountItems();++j)
			{
				if (i==j)
					continue;
					
				BMediaConstraintItem::simplify_result_t result;
				
				BMediaConstraintItem &S1=mConstraintItems.EditItemAt(i);
				BMediaConstraintItem &S2=mConstraintItems.EditItemAt(j);
				
				result=BMediaConstraintItem::Simplify(mConjunctive,
															S1,
															S2);
				
				if (mConjunctive)
				{
					if (result==BMediaConstraintItem::B_FALSE)
					{
						mConstraintItems.MakeEmpty();
						return result;
					}
					else if (result==BMediaConstraintItem::B_TRUE)
					{
						RemoveConstraintItemPair(i,j);					
						changed=true;
					}
				}
				else
				{
					if (result==BMediaConstraintItem::B_TRUE)
					{
						mConstraintItems.MakeEmpty();
						return result;
					}
					else if (result==BMediaConstraintItem::B_FALSE)
					{					
						RemoveConstraintItemPair(i,j);					
						changed=true;
					}
				}
				
				if (!changed)
				{
					switch (result)
					{
						case BMediaConstraintItem::B_KEEP_FIRST:
						{
							mConstraintItems.RemoveItemsAt(j);
							changed=true;
						}
						break;

						case BMediaConstraintItem::B_KEEP_SECOND:
						{
							mConstraintItems.RemoveItemsAt(i);
							changed=true;
						}
						break;

						case BMediaConstraintItem::B_KEEP_BOTH:
							break;
						
						default:
							TRESPASS();
							break;
					}
				}	
			}
		}
	}
	while (changed);
	
	return BMediaConstraintItem::B_NONE;
}

void 
BMediaConstraintAlternative::RemoveConstraintItemPair(size_t i, size_t j)
{
	if (i<j)
	{
		mConstraintItems.RemoveItemsAt(j);
		mConstraintItems.RemoveItemsAt(i);
	}
	else
	{
		mConstraintItems.RemoveItemsAt(i);
		mConstraintItems.RemoveItemsAt(j);
	}
}

size_t 
BMediaConstraintAlternative::CountConstraintItems() const
{
	return mConstraintItems.CountItems();
}

const BMediaConstraintItem &
BMediaConstraintAlternative::ConstraintItemAt(size_t i) const
{
	return mConstraintItems.ItemAt(i);
}

}; }; // namespace B::Media2
