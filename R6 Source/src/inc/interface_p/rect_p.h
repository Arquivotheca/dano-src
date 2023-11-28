/* private rect functions */

#if !defined(RECT_P_H)
#define RECT_P_H

#include <Rect.h>


inline BRect operator+(const BRect& a, const BPoint& b)
{
        return BRect(a.left+b.x, a.top+b.y, a.right+b.x, a.bottom+b.y);
}

inline BRect operator-(const BRect& a, const BPoint& b)
{
        return BRect(a.left-b.x, a.top-b.y, a.right-b.x, a.bottom-b.y);
}

inline BRect& operator+=(BRect& a, const BPoint& b)
{
        a.left += b.x; a.right += b.x;
        a.top += b.y; a.bottom += b.y;
        return a;
}

inline BRect& operator-=(BRect& a, const BPoint& b)
{
        a.left -= b.x; a.right -= b.x;
        a.top -= b.y; a.bottom -= b.y;
        return a;
}


#endif /* RECT_P_H */

