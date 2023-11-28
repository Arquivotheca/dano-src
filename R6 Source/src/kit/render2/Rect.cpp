
#include <render2/Rect.h>
#include <support2/ITextStream.h>

namespace B {
namespace Render2 {

void
BRect::Construct(const value_ref& ref, status_t *result)
{
	if (ref.type == B_RECT_TYPE) {
		if (ref.length == sizeof(BRect)) {
			*this = *static_cast<const BRect*>(ref.data);
			if (result) *result = B_OK;
		} else {
			if (result) *result = B_BAD_VALUE;
			*this = BRect();
		}
	} else {
		if (result) *result = B_BAD_TYPE;
		*this = BRect();
	}
}

BRect::BRect(const BValue& o, status_t *result)
{
	Construct(value_ref(o), result);
}

BRect::BRect(const value_ref& o, status_t *result)
{
	Construct(o, result);
}

BValue
BRect::AsValue() const
{
	return BValue(B_RECT_TYPE, this, sizeof(BRect));
}

BRect &
BRect::SetAsSize(coord l, coord t, coord w, coord h)
{
	left = l;
	top = t;
	right = l+w;
	bottom = t+h;
	return (*this);
}

void
BRect::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "BRect(";
	else io << "(";
	io	<< left << ","
		<< top << ")-("
		<< right << ","
		<< bottom << ")";
}

status_t
BRect::printer(ITextOutput::arg io, const value_ref& val, uint32 flags)
{
	status_t result;
	BRect obj(val, &result);
	if (result == B_OK) obj.PrintToStream(io, flags);
	return result;
}

BRect &
BRect::InsetBy(const BPoint& p)
{
	left += p.x;
	right -= p.x;
	top += p.y;
	bottom -= p.y;
	return *this;
}

BRect &
BRect::OffsetBy(const BPoint& p)
{
	left += p.x;
	right += p.x;
	top += p.y;
	bottom += p.y;
	return *this;
}

BRect &
BRect::OffsetTo(const BPoint& p)
{
	right += p.x - left;
	left = p.x;
	bottom += p.y - top;
	top = p.y;
	return *this;
}

bool
BRect::Contains(const BPoint& p) const
{
	return
		(p.x >= left) &&
		(p.x < right) &&
		(p.y >= top) &&
		(p.y < bottom);
}

bool
BRect::Contains(const BRect& r) const
{
	return(left <= r.left &&
	       r.left < r.right &&
	       r.right <= right &&
	       top <= r.top &&
	       r.top < r.bottom &&
	       r.bottom <= bottom);
}

BRect 
BRect::InsetByCopy(const BPoint& p) const
{
	return BRect(*this).InsetBy(p);
}

BRect 
BRect::OffsetByCopy(const BPoint& p) const
{
	return BRect(*this).OffsetBy(p);
}

BRect 
BRect::OffsetToCopy(const BPoint& p) const
{
	return BRect(*this).OffsetTo(p);
}

int32
BRect::Compare(const BRect& r) const
{
	if (left != r.left) return left < r.left ? -1 : 1;
	if (top != r.top) return top < r.top ? -1 : 1;
	if (right != r.right) return right < r.right ? -1 : 1;
	if (bottom != r.bottom) return bottom < r.bottom ? -1 : 1;
	return 0;
}

bool
BRect::operator==(const BRect& other) const
{
	return (
		(left==other.left) &&
		(right==other.right) &&
		(top==other.top) &&
		(bottom==other.bottom)
	);
}

bool
BRect::operator!=(const BRect& other) const
{
	return (
		(left!=other.left) ||
		(right!=other.right) ||
		(top!=other.top) ||
		(bottom!=other.bottom)
	);
}

BRect
BRect::operator&(const BRect& r) const
{
	BRect aRect;
	
	aRect.left = max_c(left, r.left);
	aRect.top = max_c(top, r.top);
	aRect.right = min_c(right, r.right);
	aRect.bottom = min_c(bottom, r.bottom);
	
	return(aRect);
}

BRect
BRect::operator|(const BRect& r) const
{
	BRect aRect;
	
	aRect.left = min_c(left, r.left);
	aRect.top = min_c(top, r.top);
	aRect.right = max_c(right, r.right);
	aRect.bottom = max_c(bottom, r.bottom);
	
	return(aRect);
}

bool
BRect::Intersects(const BRect& r) const
{
	if (top >= r.bottom)
		return(false);
	if (bottom <= r.top)
		return(false);
	if (left >= r.right)
		return(false);
	if (right <= r.left)
		return(false);

	return(true);
}

//------------------------------------------------------------------------------

BRect BRect::operator * (const coord f) const
{
	return BRect(left*f, top*f, right*f, bottom*f);
}

BRect BRect::operator / (const coord f) const
{
	return operator * (1.0f/f);
}

BRect BRect::operator + (const BPoint& p) const
{
	return BRect(left+p.x, top+p.y, right+p.x, bottom+p.y);
}

BRect BRect::operator - (const BPoint& p) const
{
	return BRect(left-p.x, top-p.y, right-p.x, bottom-p.y);
}

BRect& BRect::operator *= (const coord f)
{
	left *= f;
	top *= f;
	right *= f;
	bottom *= f;
	return *this;
}

BRect& BRect::operator /= (const coord f)
{
	return operator *= (1.0f/f);
}

BRect& BRect::operator += (const BPoint& p)
{
	left += p.x;
	right += p.x;
	top += p.y;
	bottom += p.y;
	return *this;
}

BRect& BRect::operator -= (const BPoint& p)
{
	left -= p.x;
	right -= p.x;
	top -= p.y;
	bottom -= p.y;
	return *this;
}

//------------------------------------------------------------------------------

ITextOutput::arg
operator<<(ITextOutput::arg io, const BRect &rect)
{
	rect.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

} }	// namespace B::Render2
