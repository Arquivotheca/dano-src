// TO DO: +++
// - consistent handling of mismatched types

#include "resolve_format.h"

#include <cmath>
#include <climits>

#include <media2/MediaDefs.h>
#include <media2/MediaConstraint.h>
#include <media2/MediaFormat.h>
#include <media2/MediaPreference.h>

#include <support2/KeyedVector.h>
#include <support2/CallStack.h>
#include <support2/Debug.h>
#include <support2/OrderedVector.h>

#include <support2/StdIO.h>

namespace B {
namespace Media2 {
namespace FormatResolver {

// **************************************************************** //

// relation ordering values: higher values -> broader constraints

inline int32 relation_order(BMediaConstraintItem::relation_t relation)
{
	const int32 order[] =
	{
		0,	// one-of
		-1,	// lt: range special case
		2,	// multiple-of
		1	// power-of
	};
	
	const int32 inverted_order[] =
	{
		5,	// not-one-of
		-1,	// ge: range special case
		3,	// multiple-of
		4	// power-of
	};

	return (relation & BMediaConstraintItem::B_INVERT) ?
		inverted_order[relation & ~BMediaConstraintItem::B_INVERT] :
		order[relation];
}

class _ConstraintItem : public BMediaConstraintItem
{
public:
	_ConstraintItem() : BMediaConstraintItem(), mOrder(-1) {}
	_ConstraintItem(const BMediaConstraintItem & item) :
		BMediaConstraintItem(item),
		mOrder(relation_order(item.Relation())) {}
	_ConstraintItem(const _ConstraintItem & clone) : BMediaConstraintItem()
	{
		operator=(clone);
	}
	_ConstraintItem & operator=(const _ConstraintItem & clone)
	{
		BMediaConstraintItem::operator=(clone);
		mOrder = clone.mOrder;
		return *this;
	}
	bool operator<(const _ConstraintItem & o) const
	{
		return mOrder < o.mOrder;
//		return (mOrder != o.mOrder) ? mOrder < o.mOrder : Key() < o.Key();
	}
private:
	int32	mOrder;
};

// ordered set of constraint items for a particular key,
// with a specially handled range [lt] [ge]
class _ConstraintItemSet
{
public:
	_ConstraintItemSet() : type(B_NULL_TYPE) {}

	type_code						type;
	BValue							lt;
	BValue							ge;
	BOrderedVector<_ConstraintItem>	items;
};

typedef BKeyedVector<BValue, _ConstraintItemSet> _alternative;

enum resolve_status
{
	RESOLVE_FAILED	= -1,
	RESOLVE_MATCHED_PREF,
	RESOLVE_MATCHED
};

// **************************************************************** //

void
reorder_alternative(
	const BMediaConstraintAlternative & alternative,
	_alternative & a);

int64
value_as_int64(BValue value);

double
value_as_double(BValue value);

BValue
int64_to_type(int64 value, type_code type);

BValue
double_to_type(double value, type_code type);

// unpack multiple null->value mappings into the given vector
void
unpack_value(BValue value, BOrderedVector<BValue> & out);

bool
is_numeric_type(type_code type);

// clamp the given preference to the range optionally determined by
// set.gt and set.le
BValue
resolve_LT_GE(
	const _ConstraintItemSet & set,
	BValue pref);

// resolve_from_XXX:
// the first item in set must be of the given relation type.
// walk the possible values determined by that item, starting with the
// nearest to 'pref' and moving outward, within the range determined by
// set.lt and set.ge if defined.  test remaining constraints against each
// candidate value, and return the first one that matches all constraints.

BValue
resolve_from_ONE_OF(
	const _ConstraintItemSet & set,
	BValue pref);

BValue
resolve_from_MULTIPLE_OF(
	const _ConstraintItemSet & set,
	BValue pref);

BValue
resolve_from_NOT_MULTIPLE_OF(
	const _ConstraintItemSet & set,
	BValue pref);

bool
is_double_in_set(const BOrderedVector<BValue> & set, double value, double precision);

BValue
resolve_from_NOT_ANY_OF(
	const _ConstraintItemSet & set,
	BValue pref);

// **************************************************************** //

// test_constraint_item:
// return true if the value matches the given constraint, false otherwise.

bool
test_constraint_item(
	const BMediaConstraintItem & item,
	BValue pref);

// **************************************************************** //

BValue
resolve_key(
	const _ConstraintItemSet & set,
	BValue pref);

status_t
resolve_alternative(
	const _alternative & alternative,
	const BMediaPreference & pref,
	BMediaFormat & outFormat,
	float & outScore);
	
// **************************************************************** //

int64
value_as_int64(BValue value)
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
			return int64(val + 0.5f);
		}

		case B_DOUBLE_TYPE:
		{
			double val;
			value.GetDouble(&val);
			return int64(val + 0.5);
		}
		
		default:
			TRESPASS();
			return 0;
	}
}

double
value_as_double(BValue value)
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
			return val;
		}

		case B_DOUBLE_TYPE:
		{
			double val;
			value.GetDouble(&val);
			return val;
		}
		
		default:
			TRESPASS();
			return 0.;
	}
}

BValue int64_to_type(
	int64 value,
	type_code type)
{
	switch (type)
	{
		case B_INT8_TYPE:		return BValue::Int8((int8)value);
		case B_INT16_TYPE:		return BValue::Int16((int16)value);
		case B_INT32_TYPE:		return BValue::Int32((int32)value);
		case B_INT64_TYPE:		return BValue::Int64(value);
		case B_FLOAT_TYPE:		return BValue::Float((float)value);
		case B_DOUBLE_TYPE:		return BValue::Double((double)value);
		default:				return BValue::undefined;
	}
}

BValue double_to_type(
	double value,
	type_code type)
{
	switch (type)
	{
		case B_INT8_TYPE:		return BValue::Int8((int8)(value + 0.5));
		case B_INT16_TYPE:		return BValue::Int16((int16)(value + 0.5));
		case B_INT32_TYPE:		return BValue::Int32((int32)(value + 0.5));
		case B_INT64_TYPE:		return BValue::Int64((int64)(value + 0.5));
		case B_FLOAT_TYPE:		return BValue::Float((float)value);
		case B_DOUBLE_TYPE:		return BValue::Double(value);
		default:				return BValue::undefined;
	}
}

bool 
is_numeric_type(type_code type)
{
	switch (type)
	{
		case B_INT8_TYPE:
		case B_INT16_TYPE:
		case B_INT32_TYPE:
		case B_INT64_TYPE:
		case B_FLOAT_TYPE:
		case B_DOUBLE_TYPE:
			return true;
		default:
			return false;
	}
}

void 
unpack_value(BValue value, BOrderedVector<BValue> &out)
{
	void * cookie = 0;
	BValue v;
	while (value.GetNextItem(&cookie, 0, &v) >= B_OK)
	{
		out.AddItem(v);
	}
}

void
reorder_alternative(
	const BMediaConstraintAlternative & alternative,
	_alternative & a)
{
	// build an ordered vector for each key in the alternative,
	// pulling out lt/ge values
	for (ssize_t ni = alternative.CountConstraintItems()-1; ni >= 0; ni--)
	{
		const BMediaConstraintItem & i = alternative.ConstraintItemAt(ni);

		ASSERT ((i.Relation() & ~BMediaConstraintItem::B_INVERT) !=
				BMediaConstraintItem::B_POWER_OF);
		
		if (a.IndexOf(i.Key()) < 0)
		{
			a.AddItem(i.Key(), _ConstraintItemSet());
		}
		_ConstraintItemSet & s = a.EditValueFor(i.Key());
		
		if (s.type == B_NULL_TYPE && i.Value().IsSpecified())
		{
			if (i.Value().IsSimple())
			{
				s.type = i.Value().Type();
			}
			else
			{
				void * cookie = 0;
				BValue v;
				if (i.Value().GetNextItem(&cookie, 0, &v) >= B_OK)
				{
					s.type = v.Type();
				}
				else
				{
					s.type = B_NULL_TYPE;
				}
			}
		}
		
		if (i.Relation() == BMediaConstraintItem::B_LT)
		{
			ASSERT(!s.lt.IsSpecified());
			s.lt = i.Value();
		}
		else
		if (i.Relation() == BMediaConstraintItem::B_GE)
		{
			ASSERT(!s.ge.IsSpecified());
			s.ge = i.Value();
		}
		else s.items.AddItem(_ConstraintItem(i));
	}
}

// clamp given value to the range specified in set, if any.
BValue
resolve_LT_GE(const _ConstraintItemSet & set, BValue pref)
{
	ASSERT(pref.IsDefined());
	if (set.ge.IsDefined() && pref < set.ge)
	{
		return set.ge;
	}
	if (set.lt.IsDefined() && pref >= set.lt)
	{
		return set.lt;
	}
	return pref;
}

BValue 
resolve_from_ONE_OF(const _ConstraintItemSet &set, BValue pref)
{
	const BMediaConstraintItem & base = set.items[0];
	ASSERT (base.Relation() == BMediaConstraintItem::B_ONE_OF);
	
	// range should have been simplified out:
	ASSERT (!set.lt.IsDefined());
	ASSERT (!set.ge.IsDefined());

	BOrderedVector<BValue> vv;
	unpack_value(base.Value(), vv);
	
	if (!is_numeric_type(set.type))
	{
		// find a matching value, or pick the first possibility
		for (size_t n = 0; n < vv.CountItems(); n++)
		{
			if (vv[n] == pref)
			{
				return vv[n];
			}
		}
		return (vv.CountItems() > 0) ? vv[0] : BValue::undefined;
	}
	
	// we know we're dealing with a numeric type: convert the
	// preference to double, then find the nearest allowed value
	double pref_d = value_as_double(pref);
	
	// index of current value to test
	int32 cur = -1;
	
	// search for nearest value to pref
	ssize_t low = 0, high = vv.CountItems()-1;
	double best_d = DBL_MAX;
	while (low <= high)
	{
		ssize_t mid = (low + high) / 2;
		double d = pref_d - value_as_double(vv[mid]);
		double ad = fabs(d);
		if (cur < 0 || ad < best_d)
		{
			cur = mid;
			best_d = ad;
		}
		if (d < 0.0) high = mid - 1;
		else if (d > 0.0) low = mid + 1;
		else break;
	}
	
	ASSERT (cur >= 0);

	// pick next highest and next lowest values
	int32 next_low = cur-1;
	int32 next_high = cur+1;
	
	// test current value; return if it matches
	// else pick next nearest value; return undefined when exhausted
	while (true)
	{
		bool matched = true;
		for (size_t n = 1; n < set.items.CountItems(); n++)
		{
			if (!test_constraint_item(set.items[n], vv[cur]))
			{
				matched = false;
				break;
			}
		}
		if (matched) return vv[cur];
		
		double d_low = (next_low >= 0) ?
			fabs(value_as_double(vv[cur]) - value_as_double(vv[next_low])) :
			-1.0;
		double d_high = (next_high < (int32)vv.CountItems()) ?
			fabs(value_as_double(vv[cur]) - value_as_double(vv[next_high])) :
			-1.0;
		
		if (d_low >= 0.0)
		{
			if (d_high >= 0.0 && d_high < d_low)
			{
				cur = next_high++;
			}
			else
			{
				cur = next_low--;
			}
		}
		else
		if (d_high >= 0.0)
		{
			cur = next_high++;
		}
		else
		{
			return BValue::undefined;
		}
	}
}

static int64 llabs(int64 v) { return (v >= 0LL) ? v : -v; }

BValue 
resolve_from_MULTIPLE_OF(const _ConstraintItemSet &set, BValue pref)
{
	if (!is_numeric_type(set.type))
	{
		debugger("resolve_from_MULTIPLE_OF: expected numeric value");
		return BValue::undefined;
	}
	
	const BMediaConstraintItem & base = set.items[0];
	ASSERT (base.Relation() == BMediaConstraintItem::B_MULTIPLE_OF);
	
	const int64 lt = (set.lt.IsDefined()) ? value_as_int64(set.lt) : LONGLONG_MAX;
	const int64 ge = (set.ge.IsDefined()) ? value_as_int64(set.ge) : LONGLONG_MIN;

	// clamp preferred value to range, convert to int64
	int64 pref_v = value_as_int64(pref);
	int64 cur = pref_v;
	if (cur >= lt) cur = lt;
	if (cur < ge) cur = ge;

	// multiplier
	const int64 inc = value_as_int64(base.Value());

	// find nearest multiple in range
	int64 cur_offset = cur % inc;
	if (cur_offset)
	{
		int64 low = cur - cur_offset;
		int64 high = cur + (inc - cur_offset);
		
		if (low < ge)
		{
			// low out of range
			if (high >= lt)
			{
				// both out of range, ouch
berr << "resolve_from_MULTIPLE_OF failed (a): pref " << pref << ", inc " << inc << ", (>= " <<
	set.ge << ") (< " << set.lt << ")" << endl;
				return BValue::undefined;
			}
			cur = high;
		}
		else
		if (high >= lt)
		{
			// high out of range
			cur = low;
		}
		else
		{
			// both in range, pick the nearest
			int64 d_low = llabs(pref_v - low);
			int64 d_high = llabs(pref_v - high);
			cur = (d_low < d_high) ? low : high;
		}
	}

	int64 next_low = cur - inc;
	int64 next_high = cur + inc;
	
	// test current value; return if it matches
	// else pick next nearest value; return undefined when exhausted
	while (true)
	{
		// convert current value back to original type
		BValue cur_value(int64_to_type(cur, set.type));

		// test against remaining constraints
		bool matched = true;
		for (size_t n = 1; n < set.items.CountItems(); n++)
		{
			if (!test_constraint_item(set.items[n], cur_value))
			{
				matched = false;
				break;
			}
		}
		if (matched) return cur_value;
	
		// find another value in range; if two choices are available, pick the
		// closer one to the original preferred value
		if (next_low < ge)
		{
			if (next_high >= lt)
			{	
berr << "resolve_from_MULTIPLE_OF failed (b): pref " << pref << ", cur " << cur <<
	", inc " << inc << ", (>= " << set.ge << ") (< " << set.lt << ")" << endl;
				return BValue::undefined;
			}
			cur = next_high;
			next_high += inc;
		}
		else
		if (next_high >= lt)
		{
			cur = next_low;
			next_low -= inc;
		}
		else
		{
			int64 d_low = llabs(next_low - pref_v);
			int64 d_high = llabs(next_high - pref_v);
			if (d_low < d_high)
			{
				cur = next_low;
				next_low -= inc;
			}
			else
			{
				cur = next_high;
				next_high += inc;
			}
		}
	}
}

BValue 
resolve_from_NOT_MULTIPLE_OF(const _ConstraintItemSet &set, BValue pref)
{
	if (!is_numeric_type(set.type))
	{
		debugger("resolve_from_NOT_MULTIPLE_OF: expected numeric value");
		return BValue::undefined;
	}
	
	const BMediaConstraintItem & base = set.items[0];
	ASSERT (base.Relation() == BMediaConstraintItem::B_NOT_MULTIPLE_OF);
	
	const int64 lt = (set.lt.IsDefined()) ? value_as_int64(set.lt) : LONGLONG_MAX;
	const int64 ge = (set.ge.IsDefined()) ? value_as_int64(set.ge) : LONGLONG_MIN;

	// clamp preferred value to range, convert to int64
	int64 pref_v = value_as_int64(pref);
	int64 cur = pref_v;
	if (cur >= lt) cur = lt;
	if (cur < ge) cur = ge;

	// multiplier
	const int64 inc = value_as_int64(base.Value());
	if (inc == 1)
	{
		// no possible solution
berr << "resolve_from_NOT_MULTIPLE_OF: denominator is 1!" << endl;
		return BValue::undefined;
	}

	// find nearest non-multiple in range
	int64 cur_offset = cur % inc;
	if (!cur_offset)
	{
		int64 low = cur - 1;
		int64 high = cur + 1;
		
		if (low < ge)
		{
			// low out of range
			if (high >= lt)
			{
				// both out of range, ouch
berr << "resolve_from_NOT_MULTIPLE_OF failed (a): pref " << pref << ", inc " << inc << ", (>= " <<
	set.ge << ") (< " << set.lt << ")" << endl;
				return BValue::undefined;
			}
			cur = high;
		}
		else
		if (high >= lt)
		{
			// high out of range
			cur = low;
		}
		else
		{
			// both in range, pick the nearest
			int64 d_low = llabs(pref_v - low);
			int64 d_high = llabs(pref_v - high);
			cur = (d_low < d_high) ? low : high;
		}
	}

	int64 next_low = cur - 1;
	int64 next_high = cur + 1;
	
	// test current value; return if it matches
	// else pick next nearest value; return undefined when exhausted
	while (true)
	{
		// convert current value back to original type
		BValue cur_value(int64_to_type(cur, set.type));

		// test against remaining constraints
		bool matched = true;
		for (size_t n = 1; n < set.items.CountItems(); n++)
		{
			if (!test_constraint_item(set.items[n], cur_value))
			{
				matched = false;
				break;
			}
		}
		if (matched) return cur_value;
	
		// find another value in range; if two choices are available, pick the
		// closer one to the original preferred value
		if (next_low < ge)
		{
			if (next_high >= lt)
			{	
berr << "resolve_from_NOT_MULTIPLE_OF failed (b): pref " << pref << ", cur " << cur <<
	", inc " << inc << ", (>= " << set.ge << ") (< " << set.lt << ")" << endl;
				return BValue::undefined;
			}
			cur = next_high;
			while (!(++next_high % inc)) {}
		}
		else
		if (next_high >= lt)
		{
			cur = next_low;
			while (!(--next_low % inc)) {}
		}
		else
		{
			int64 d_low = llabs(next_low - pref_v);
			int64 d_high = llabs(next_high - pref_v);
			if (d_low < d_high)
			{
				cur = next_low;
				while (!(--next_low % inc)) {}
			}
			else
			{
				cur = next_high;
				while (!(++next_high % inc)) {}
			}
		}
	}
}

bool
is_double_in_set(const BOrderedVector<BValue> & set, double value, double precision)
{
	ssize_t low = 0, high = set.CountItems()-1;
	while (low <= high)
	{
		ssize_t mid = (low + high) / 2;
		double d = value - value_as_double(set[mid]);
		double ad = fabs(d);
		if (ad < precision) return true;
		else if (d < 0.0) high = mid - 1;
		else low = mid + 1;
	}
	return false;
}

BValue 
resolve_from_NOT_ANY_OF(const _ConstraintItemSet &set, BValue pref)
{
	if (!is_numeric_type(set.type))
	{
		debugger("resolve_from_NOT_ANY_OF: expected numeric value");
		return BValue::undefined;
	}

	const BMediaConstraintItem & base = set.items[0];
	ASSERT (base.Relation() == BMediaConstraintItem::B_NOT_ANY_OF);
	
	const double lt = (set.lt.IsDefined()) ? value_as_double(set.lt) : DBL_MAX;
	const double ge = (set.ge.IsDefined()) ? value_as_double(set.ge) : DBL_MIN;
	// how close do we consider a match to be?
	const double precision = 0.001;
	// how far do we jump at a time?
	const double step = 1.0;

	BOrderedVector<BValue> vv;
	unpack_value(base.Value(), vv);
	
	// clamp preferred value to range
	double pref_d = value_as_double(pref);
	double cur = pref_d;
	if (cur >= lt) cur = lt;
	if (cur < ge) cur = ge;
	
	// search for a value not in the set, maintaining next-lowest and next-highest values
	// for use when continuing the search later
	double next_low = cur - step;
	double next_high = cur + step;
	while (is_double_in_set(vv, cur, precision))
	{
		if (next_low < ge)
		{
			if (next_high >= lt)
			{ 
berr << "resolve_from_NOT_ANY_OF failed (a): pref " << pref << ", cur " << cur << ", (>= " <<
	set.ge << ") (< " << set.lt << ")" << endl;
				return BValue::undefined;
			}
			cur = next_high;
			next_high += step;
		}
		else
		if (next_high >= lt)
		{
			cur = next_low;
			next_low -= step;
		}
		else
		{
			double d_low = fabs(next_low - pref_d);
			double d_high = fabs(next_high - pref_d);
			if (d_low < d_high)
			{
				cur = next_low;
				next_low -= step;
			}
			else
			{
				cur = next_high;
				next_high += step;
			}
		}
	}
	
	// test current value; return if it matches
	// else pick next nearest value; return undefined when exhausted
	while (true)
	{
		// convert current value back to original type
		// +++ this worries me precision-wise
		BValue cur_value(double_to_type(cur, set.type));

		bool matched = true;
		for (size_t n = 1; n < set.items.CountItems(); n++)
		{
			if (!test_constraint_item(set.items[n], cur_value))
			{
				matched = false;
				break;
			}
		}
		if (matched) return cur_value;

		// search for the nearest of the next values not in the set
		while (next_low >= ge && is_double_in_set(vv, next_low, precision)) next_low -= step;
		while (next_high < lt && is_double_in_set(vv, next_high, precision)) next_high += step;

		if (next_low < ge)
		{
			if (next_high >= lt)
			{
berr << "resolve_from_NOT_ANY_OF failed (b): pref " << pref << ", cur " << cur <<
	", (>= " << set.ge << ") (< " << set.lt << ")" << endl;
				return BValue::undefined;
			}
			cur = next_high;
			next_high += step;
		}
		else
		if (next_high >= lt)
		{
			cur = next_low;
			next_low -= step;
		}
		else
		{
			double d_low = fabs(next_low - pref_d);
			double d_high = fabs(next_high - pref_d);
			if (d_low < d_high)
			{
				cur = next_low;
				next_low -= step;
			}
			else
			{
				cur = next_high;
				next_high += step;
			}
		}			
	}
}

bool 
test_constraint_item(const BMediaConstraintItem &item, BValue pref)
{
	switch (item.Relation())
	{
		case BMediaConstraintItem::B_ONE_OF:
		{
			void * cookie = 0;
			BValue v;
			while (item.Value().GetNextItem(&cookie, 0, &v) >= B_OK)
			{
				if (v == pref) return true;
			}
			return false;
		}
		
		case BMediaConstraintItem::B_MULTIPLE_OF:
		{
			int64 pref_v = value_as_int64(pref);
			return !(pref_v % value_as_int64(item.Value()));
		}

		case BMediaConstraintItem::B_NOT_ANY_OF:
		{
			void * cookie = 0;
			BValue v;
			while (item.Value().GetNextItem(&cookie, 0, &v) >= B_OK)
			{
				if (v == pref) return false;
			}
			return true;
		}
		
		case BMediaConstraintItem::B_NOT_MULTIPLE_OF:
		{
			int64 pref_v = value_as_int64(pref);
			return (pref_v % value_as_int64(item.Value()));
		}
		
		default:
			TRESPASS();
			return false;
	}
}

BValue
resolve_key(const _ConstraintItemSet & set, BValue pref)
{
	if (!set.items.CountItems())
	{
		return resolve_LT_GE(set, pref);
	}
	switch (set.items[0].Relation())
	{
		case BMediaConstraintItem::B_ONE_OF:
			return resolve_from_ONE_OF(set, pref);

		case BMediaConstraintItem::B_MULTIPLE_OF:
			return resolve_from_MULTIPLE_OF(set, pref);

		case BMediaConstraintItem::B_NOT_ANY_OF:
			return resolve_from_NOT_ANY_OF(set, pref);

		case BMediaConstraintItem::B_NOT_MULTIPLE_OF:
			return resolve_from_NOT_MULTIPLE_OF(set, pref);
		
		default:
			TRESPASS();
			return BValue::undefined;
	}
}

status_t
resolve_alternative(
	const _alternative & alternative,
	const BMediaPreference & pref,
	BMediaFormat & outFormat,
	float & outScore)
{
	for (ssize_t nk = alternative.CountItems()-1; nk >= 0; nk--)
	{
		const BValue & key = alternative.KeyAt(nk);
		const _ConstraintItemSet & set = alternative.ValueAt(nk);
		
		// try each preference for this key, retaining the highest-scoring
		// value (score = weight if a preference is matched, 0 otherwise)
		BValue bestValue;
		float bestScore = -1.0f;
		
		bool foundPref = false;
		void * cookie = 0;
		BValue pref_v;
		float weight;
		while (pref.GetNextItem(key, &cookie, &pref_v, &weight) == B_OK)
		{
			foundPref = true;
			BValue v = resolve_key(set, pref_v);
			if (v == BValue::undefined)
			{
				if (weight > bestScore)
				{
					bestValue = pref_v;
					bestScore = weight;
				}
			}
			else
			{
				float score = (v == pref_v) ? weight : 0.0f;
				if (score > bestScore)
				{
					bestValue = v;
					bestScore = score;
				}
			}
		}
		
		if (!foundPref)
		{
			if (set.type == B_NULL_TYPE)
			{
berr << "resolve_alternative(): no constraint key and no preference for key " << key << endl;
				return B_ERROR;
			}
			bestValue = resolve_key(set, int64_to_type(0LL, set.type));
			if (bestValue == BValue::undefined)
			{
berr << "resolve_alternative(): resolve_key undefined for key " << key << " (default pref)" << endl;
				return B_ERROR;
			}
			bestScore = 0.0f;
		}

		outFormat.Overlay(key, bestValue);
		outScore += bestScore;
	}
}

} // namespace FormatResolver

//using namespace FormatResolver;

status_t 
resolve_format(
	const BMediaConstraint &constraint,
	BMediaPreference const * const * prefs, size_t pref_count,
	BMediaFormat * outFormat)
{
	BMediaFormat best;
	float bestScore = -1.0f;
	
	for (size_t na = 0; na < constraint.CountAlternatives(); na++)
	{
		// order by key, then relation
		const BMediaConstraintAlternative & alternative = constraint.AlternativeAt(na);
		FormatResolver::_alternative a;
		FormatResolver::reorder_alternative(alternative, a);
		
		BMediaPreference unconstrained;
	
		// add empty entries for any additional keys ref'd by prefs
		for (size_t np = 0; np < pref_count; np++)
		{
			void * cookie = 0;
			BValue key;
			while (prefs[np]->GetNextKey(&cookie, &key) >= B_OK)
			{
				if (a.IndexOf(key) < 0)
				{
					// this key isn't constrained, so keep track of all the preferred
					// values for it
					void * pref_cookie = 0;
					BValue value;
					float weight;
					while (prefs[np]->GetNextItem(key, &pref_cookie, &value, &weight) >= B_OK)
					{
						unconstrained.AddItem(key, value, weight);
					}

					// add a key
					a.AddItem(key, FormatResolver::_ConstraintItemSet());
				}
			}
		}
		
		// test each preference
		for (size_t np = 0; np < pref_count; np++)
		{
			BMediaFormat format;
			float score = 0.0f;
			BMediaPreference p = *prefs[np];
			p.Overlay(unconstrained);
			status_t err = FormatResolver::resolve_alternative(a, p, format, score);
			if (err < B_OK) return err;
			if (score > bestScore)
			{
				best = format;
				bestScore = score;
			}
		}
	}
	*outFormat = best;
	return B_OK;
}

} } // B::Media2
