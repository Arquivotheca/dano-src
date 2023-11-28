/***************************************************************************
//
//	File:			render2/Update.h
//
//	Description:	A object to describe an update in terms of pixels to
//					move and pixels to invalidate
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _RENDER2_UPDATE_RESOLVER_H_
#define _RENDER2_UPDATE_RESOLVER_H_

#include <render2/Point.h>
#include <render2/Rect.h>
#include <render2/Region.h>
#include <render2/2dTransform.h>
#include <support2/Flattenable.h>
#include <support2/Vector.h>

namespace B {
namespace Render2 {

using namespace B::Support2;


class BUpdate : public BFlattenable
{
public:	
	enum { // Hint flags
		B_OPAQUE			= 0x00000000, 					// opaque surface
		B_TRANSLUCENT		= 0x00000001,					// translucent surface
		B_TRANSPARENT		= 0x00000002 | B_TRANSLUCENT,	// fully transparent surface (not used yet)
		B_UPDATE_ON_RESIZE	= 0x00000004, 					// update when surface resized (ie: shape changed)
		B_ALWAYS_UPDATE		= 0x00000008	 				// this surface will always be updated
	};

	struct record {
		record() { };
		record(const BRegion& r, const BPoint& p)
			: offset(p), region(r) { };
		BPoint		offset;
		BRegion		region;
	};

		// empty update. do nothing, but is usefull to compose with.
		static const BUpdate empty;

	////////////////////////////////////////////////////////
	// Constructors

		BUpdate();

		BUpdate(const uint32 flags,
				const B2dTransform& t0, const BRegion& r0,
				const B2dTransform& t1, const BRegion& r1);

		BUpdate(const uint32 flags,
				const B2dTransform& t0, const BRegion& shape,
				const BRegion& invalidate = BRegion::empty);

		BUpdate(const BValue&);
		BUpdate(const value_ref& value);

	BValue AsValue() const;
	
	
	void MakeEmpty();
	void Transform(const B2dTransform& t);

	const bool IsEmpty() const;
	const int32 CountRegions() const { return m_movedRegions.CountItems(); }
	const BVector<record>& MovedRegions() const { return m_movedRegions; }
	const BRegion& DirtyRegion() const { return m_dirtyRegion; }

	////////////////////////////////////////////////////////
	// Compose update descriptions

	BUpdate& ComposeInFront(const BUpdate& back) {
		const BUpdate& front = *this;
		*this = compose(front, back);
		return *this;
	}

	BUpdate& ComposeBehind(const BUpdate& front) {
		const BUpdate& back = *this;
		*this = compose(front, back);
		return *this;
	}
	
	// compose a parent with a desription of all its children
	BUpdate& ComposeWithChildren(const BUpdate& children);

	// compose two same level description, in time
	BUpdate& ComposeAfter(const BUpdate& first) {
		const BUpdate& next = *this;
		*this = compose_in_time(next, first);
		return *this;
	}

	BUpdate& ComposeBefore(const BUpdate& next) {
		const BUpdate& first = *this;
		*this = compose_in_time(next, first);
		return *this;
	}
	

	////////////////////////////////////////////////////////
	// Implement BFlattenable API.
	virtual	bool		IsFixedSize() const;
	virtual	type_code	TypeCode() const;
	virtual	ssize_t		FlattenedSize() const;
	virtual	status_t	Flatten(void *buffer, ssize_t size) const;
	virtual	bool		AllowsTypeCode(type_code code) const;
	virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);

private:
	// compose 2 sibling update descriptions (automaticaly does the conversiont to parent's CS)
	static BUpdate compose(const BUpdate& front, const BUpdate& back);
	static BUpdate compose_in_time(const BUpdate& next, const BUpdate& first);
	record& operator [] (int32 index) {	return m_movedRegions.EditItemAt(index); }
	void add_record(const record& r);
	struct reg_sizes_t {
		ssize_t clean;
		ssize_t trans;
		ssize_t dirty;
		ssize_t nshap;
	};
	BVector<record>		m_movedRegions;	// array of region to move
	BRegion				m_cleanRegion;	// region covered by all visited views
	BRegion				m_translucent;	// translucent region of the surface
	BRegion				m_dirtyRegion;	// region to send an update for
	BRegion				m_newRegion;	// final shape of the view
	B2dTransform		m_srcTransform;
	B2dTransform		m_dstTransform;
};

} } // namespace B::Render2

#endif 	/* _RENDER2_UPDATE_RESOLVER_H_ */

