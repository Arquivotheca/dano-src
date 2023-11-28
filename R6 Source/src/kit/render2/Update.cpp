/***************************************************************************
//
//	File:			render2/Update.cpp
//
//	Description:	A object to describe an update in terms of pixels to
//					move and pixels to invalidate
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#include <render2/Update.h>
#include <render2/2dTransform.h>
#include <support2/TypeConstants.h>
#include <support2/StdIO.h>

namespace B {
namespace Render2 {

using namespace B::Support2;


#define DEBUG_TRANSFORM		1

#if DEBUG_TRANSFORM
#	define CHECK_TRANSFORM_TOO_COMPLEX()	(bout << __PRETTY_FUNCTION__ << ": not B_TRANSFORM_TRANSLATE only\n");
#else
#	define CHECK_TRANSFORM_TOO_COMPLEX()	;
#endif

////////////////////////////////////////////////////////

namespace BPrivate {
	// Just a little helper function
	BRegion operator + (const BRegion& r, const BPoint& p) {
		BRegion region(r);
		region.OffsetBy(p);
		return region;
	}

	BRegion operator - (const BRegion& r, const BPoint& p) {
		BRegion region(r);
		region.OffsetBy(BPoint(-p.x, -p.y));
		return region;
	}
	
	B2dTransform operator - (const B2dTransform& a, const B2dTransform& b) {
		return b.Invert()*a;
	}
}

using BPrivate::operator +;
using BPrivate::operator -;

////////////////////////////////////////////////////////
// #pragma mark -

const BUpdate BUpdate::empty = BUpdate(	BUpdate::B_OPAQUE, B2dTransform::MakeIdentity(),
										BRegion(), BRegion());

BUpdate::BUpdate()
	: BFlattenable()
{
}

BUpdate::BUpdate(const value_ref& value)
	: BFlattenable()
{
	Unflatten(value.type, value.data, value.length);
}

BUpdate::BUpdate(const BValue& value)
	: BFlattenable()
{
	value_ref ref(value);
	Unflatten(ref.type, ref.data, ref.length);
}

BUpdate::BUpdate(const uint32 flags, const B2dTransform& t0, const BRegion& shape, const BRegion& invalidate)
	:	BFlattenable(),
		m_cleanRegion(shape),
		m_newRegion(shape),
		m_srcTransform(t0),
		m_dstTransform(t0)
{
	if (flags & B_ALWAYS_UPDATE)	m_dirtyRegion = m_newRegion;
	else							m_dirtyRegion = m_newRegion & invalidate;
	if (flags & B_TRANSLUCENT)
		m_translucent = m_newRegion;
}

BUpdate::BUpdate(	const uint32 flags,
					const B2dTransform& t0, const BRegion& r0,
					const B2dTransform& t1, const BRegion& r1)
	:	BFlattenable(),
		m_cleanRegion(r0),
		m_newRegion(r1),
		m_srcTransform(t0),
		m_dstTransform(t1)
{
	// process B_ALWAYS_UPDATE first
	if (flags & B_ALWAYS_UPDATE) {
		m_newRegion.Transform(t1-t0);
		m_dirtyRegion = r0 | m_newRegion;
		if (flags & B_TRANSLUCENT)
			m_translucent = m_newRegion;
		return;
	}

	// Nothing changed
	if ((t0 == t1) && (r0 == r1)) {
		if (flags & B_TRANSLUCENT)
			m_translucent = r0;
		return;
	}

	B2dTransform dTrans = t1-t0;
	m_newRegion.Transform(dTrans);

	// Then, make sure we can handle these transformations
	if (dTrans.Operations() & ~(B_TRANSFORM_TRANSLATE))
	{ // The 2D transformation from the parent is too complex.
		// Send an update for the whole 'thing'
		CHECK_TRANSFORM_TOO_COMPLEX();
		m_dirtyRegion = (r0 | m_newRegion);
		if (flags & B_TRANSLUCENT)
			m_translucent = m_newRegion;
		return;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// From here we know that the transformations are just offsets
	// An optimisation would be to also accept scalling if the same scale factor is used
	//////////////////////////////////////////////////////////////////////////////////////////////////

	// update on resize case
	if (flags & B_UPDATE_ON_RESIZE) {
		if (r0 != r1) { // The shape changed. Update all!
			m_dirtyRegion = r0 | m_newRegion;
			if (flags & B_TRANSLUCENT)
				m_translucent = m_newRegion;
			return;
		}
	}

	// The surface didn't move
	const bool move = (dTrans.Origin() != B_ORIGIN);
	if (!move) {
		// It didn't move though, at least do not update the common parts
		m_dirtyRegion = r0 ^ m_newRegion;
		if (flags & B_TRANSLUCENT)
			m_translucent = m_newRegion;
		return;
	}

	// The surface has moved and is transluscent
	if (flags & B_TRANSLUCENT) {
		m_translucent = m_newRegion;
		m_dirtyRegion = r0 | m_newRegion;
		return;
	}

	// The surface moved  and is not translucent
	BRegion copy = m_newRegion;	// Region to be copied
	copy.Transform(dTrans.Invert());
	copy &= r0;
	m_movedRegions.AddItem(record(copy, dTrans.Origin()));
	m_dirtyRegion = (r0 - m_newRegion);	// Now compute the regions to be updated
}


////////////////////////////////////////////////////////
// #pragma mark -

void BUpdate::MakeEmpty()
{
	*this = BUpdate::empty;
}

const bool BUpdate::IsEmpty() const
{
	// Is there something to do in this update description?
	return ((DirtyRegion().IsEmpty()) && (CountRegions() == 0));
}

BUpdate& BUpdate::ComposeWithChildren(const BUpdate& children)
{
	const BUpdate& p = *this;
	BUpdate	c(children);
	
	// Then, make sure we can handle the parent's transformation
	const B2dTransform dTrans = p.m_dstTransform - p.m_srcTransform;
	if (dTrans.Operations() & ~(B_TRANSFORM_TRANSLATE)) {
		CHECK_TRANSFORM_TOO_COMPLEX();
		return *this;
	}

	// Convert the child to the parent's source coordinate system
	c.Transform(c.m_srcTransform);
	c.m_srcTransform = p.m_srcTransform;
	c.m_dstTransform = p.m_dstTransform;
	c.m_cleanRegion = c.m_newRegion;

	// The parent didn't move
	const bool move = (dTrans.Origin() != B_ORIGIN);
	if (!move) {
		c.m_dirtyRegion &= p.m_newRegion;	// revisit, I'm not sure this is correct
		c.m_cleanRegion &= p.m_newRegion;
		c.m_translucent &= p.m_newRegion;
		int32 count = c.CountRegions();
		for (int32 i=0 ; i<count ; i++) {
			c.m_movedRegions.EditItemAt(i).region &= p.m_newRegion;
			if (c.m_movedRegions[i].region.IsEmpty()) {
				c.m_movedRegions.RemoveItemsAt(i);
				i--; count--;
			}
		}
	}
	else
	{ // the parent parent moved
		const B2dTransform& t = dTrans;
		const B2dTransform ti = dTrans.Invert();
		c.m_dirtyRegion.Transform(t);
		c.m_cleanRegion.Transform(t);
		c.m_translucent.Transform(t);
		c.m_dirtyRegion &= p.m_newRegion;	// revisit, I'm not sure this is correct
		c.m_cleanRegion &= p.m_newRegion;
		c.m_translucent &= p.m_newRegion;
		int32 count = c.CountRegions();
		for (int32 i=0 ; i<count ; i++) {
			BRegion tmp = c.m_movedRegions[i].region;
			tmp.Transform(t);
			tmp &= p.m_newRegion;
			if (tmp.IsEmpty()) {
				c.m_movedRegions.RemoveItemsAt(i);
				i--; count--;
			} else {
				tmp.Transform(ti);
				c.m_movedRegions.EditItemAt(i) = record(tmp, t.Transform(c.m_movedRegions[i].offset));
			}
		}
		if (p.CountRegions()) {
			record precord = p.m_movedRegions[0];
			BRegion sources;
			BRegion dests;
			for (int32 i=0 ; i<c.CountRegions() ; i++) {
				sources |= c.m_movedRegions[i].region;
				dests |= (c.m_movedRegions[i].region + c.m_movedRegions[i].offset);
			}
			sources.Transform(ti);
			precord.region -= sources;
			dests.Transform(ti);
			precord.region -= dests;
			if (precord.region.IsEmpty() == false)
				c.add_record(precord);
		}
	}
	// Combine with the parent
	const BRegion opaqueChild = c.m_cleanRegion - c.m_translucent;
	c.m_translucent = p.m_translucent - opaqueChild;
	c.m_dirtyRegion |= p.m_dirtyRegion - opaqueChild;
	c.m_cleanRegion = p.m_newRegion;
	c.m_newRegion = p.m_newRegion;

	return (*this = c);
}


BUpdate BUpdate::compose(const BUpdate& f, const BUpdate& b)
{
	BUpdate front(f);
	BUpdate back(b);
	BUpdate result;
	int32 count;

	// Use the parent's coordinate space for the composition
	front.Transform(front.m_srcTransform);
	back.Transform(back.m_srcTransform);
	result.m_srcTransform = B2dTransform::MakeIdentity();
	result.m_dstTransform = B2dTransform::MakeIdentity();

	// Compute the new clean region for 'front', 'back' and result
	const BRegion& frontCleanRegion = front.m_newRegion;
	const BRegion& backCleanRegion = back.m_newRegion;
	result.m_cleanRegion = frontCleanRegion | backCleanRegion;
	result.m_newRegion = result.m_cleanRegion;

	// Update the translucent region
	const BRegion opaqueFront = frontCleanRegion - front.m_translucent;
	const BRegion opaqueBack = backCleanRegion - back.m_translucent;
	result.m_translucent = (front.m_translucent - opaqueBack) | (back.m_translucent - opaqueFront);

	// ---------------------------------------------------------------------------
	//	Simplify the 'back' region as possible
	// ---------------------------------------------------------------------------

	// If any of the back-dest regions intersect the new front clean_region
	// then we can remove this intersection unless it intersects the front's
	// translucent region.
	count = back.CountRegions();
	for (int i=0 ; i<count ; i++) {
		const BPoint& offset = back.m_movedRegions[i].offset;
		const BRegion dest = back.m_movedRegions[i].region + offset;
		// First process the transparent parts
		const BRegion new_dirty = dest & front.m_translucent;
		if (new_dirty.IsEmpty() == false) {
			result.m_dirtyRegion |= new_dirty;
			back[i].region -= (new_dirty - offset);
			if (back.m_movedRegions[i].region.IsEmpty()) {
				back.m_movedRegions.RemoveItemsAt(i);
				count--; i--;
				continue;
			}
		}
		// Then the overlapping parts
		const BRegion r = dest & frontCleanRegion;
		if (r.IsEmpty() == false) {
			back[i].region -= (r - offset);
			if (back.m_movedRegions[i].region.IsEmpty()) {
				back.m_movedRegions.RemoveItemsAt(i);
				count--; i--;
			}
		}
	}

	// Merge the regions that move by the same offset and remove them from the back list
	for (int32 i=0 ; i<front.CountRegions() ; i++) {
		const BRegion& region = front.m_movedRegions[i].region;
		const BPoint& offset = front.m_movedRegions[i].offset;
		const uint32 bcount = back.CountRegions();
		for (uint32 j=0 ; j<bcount ; j++) {
			if (back.m_movedRegions[j].offset == offset) {
				record rec((region | back.m_movedRegions[j].region), offset);
				result.m_movedRegions.AddItem(rec);
				back.m_movedRegions.RemoveItemsAt(j);
				break;
			}
		}
	}

	// ---------------------------------------------------------------------------
	//	Process the front region
	// ---------------------------------------------------------------------------
	// The front list is kept as is.	
	// Also build a region that covers old shape + front-dest
	BRegion frontSrcDst = front.m_cleanRegion;
	for (int32 i=0 ; i<front.CountRegions() ; i++) {
		result.add_record(front.m_movedRegions[i]);		
		frontSrcDst |= front.m_movedRegions[i].region + front.m_movedRegions[i].offset;
	}

	// Remove frontSrcDst from back-sources, and generate the corresponding update
	count = back.CountRegions();
	for (int32 i=0 ; i<count ; i++) {
		BRegion commun = (back.m_movedRegions[i].region & frontSrcDst) + back.m_movedRegions[i].offset;
		result.m_dirtyRegion |= commun;
		back[i].region -= frontSrcDst;
		if (back.m_movedRegions[i].region.IsEmpty()) {
			back.m_movedRegions.RemoveItemsAt(i);
			count--; i--;
		}
	}

	// Keep all remaining back region
	BRegion backDest;
	for (int32 i=0 ; i<back.CountRegions() ; i++) {
		if (back.m_movedRegions[i].region.IsEmpty() == false) {
			result.add_record(back.m_movedRegions[i]);
			backDest |= back.m_movedRegions[i].region + back.m_movedRegions[i].offset;
		}
	}

	// remove any (0,0) move
	count = result.CountRegions();
	for (int32 i=0 ; i<count ; i++) {
		if (result.m_movedRegions[i].offset == B_ORIGIN) {
			result.m_movedRegions.RemoveItemsAt(i);
			break;
		}
	}

	// ---------------------------------------------------------------------------
	//	Compute the new dirty region
	// ---------------------------------------------------------------------------
	// Merge the dirty regions:
	// (1) Remove the front clean regions from the dirty regions (but keep the part under translucency)
	// (2) Remove the back dest regions that don't interesects with the front's clean region
	const BRegion backDirty = back.m_dirtyRegion - (frontCleanRegion - front.m_translucent);
	const BRegion frontDirty = front.m_dirtyRegion - (backDest - frontCleanRegion);
	result.m_dirtyRegion |= backDirty | frontDirty;
	return result;
}


BUpdate BUpdate::compose_in_time(const BUpdate& n, const BUpdate& f)
{
	BUpdate first(f);
	BUpdate next(n);
	BUpdate result;
	int32 count;
		
	// new dirty region
	result.m_dirtyRegion = first.m_dirtyRegion | next.m_dirtyRegion;

	// The moved regions in commun can be merged
	count = first.CountRegions();
	for (int32 i=0 ; i<count ; i++) {
		bool region_i_empty = false;

		const BPoint& offset = first.m_movedRegions[i].offset;
		const BRegion& region = first.m_movedRegions[i].region;
		int32 nc = next.CountRegions();
		for (int32 j=0 ; j<nc ; j++) {
			const BRegion commun = region & (next.m_movedRegions[j].region - offset);
			if (commun.IsEmpty() == false) {
				const BPoint new_offset = offset + next.m_movedRegions[j].offset;
				if (new_offset != B_ORIGIN) {
					result.add_record(record(commun, new_offset));
				}

				// We know that the commun moves _are_ clean, so remove them from the dirty region
				result.m_dirtyRegion -= ((commun + new_offset) & first.m_dirtyRegion);

				// the commun region isn't owned by 'first' or 'next' anymore...
				next.m_movedRegions.EditItemAt(j).region -= commun;
				first.m_movedRegions.EditItemAt(i).region -= commun;
			}
			
			// same sources excluded from 'next' and replaced by update
			const BRegion commun_src = region & next.m_movedRegions[j].region;
			result.m_dirtyRegion |= (commun_src + next.m_movedRegions[j].offset);
			next.m_movedRegions.EditItemAt(j).region -= commun_src;

			// same destinations excluded from 'first'
			const BRegion commun_dst = (region + offset) & (next.m_movedRegions[j].region + next.m_movedRegions[j].offset);
			first.m_movedRegions.EditItemAt(i).region -= (commun_dst - offset);

			// Finaly remove everything if the region became empty
			if (next.m_movedRegions[j].region.IsEmpty()) {
				next.m_movedRegions.RemoveItemsAt(j);
				nc--; j--;
			}
			if (first.m_movedRegions[i].region.IsEmpty()) {
				first.m_movedRegions.RemoveItemsAt(i);
				--count;
				--i;
				// This used to be done with a goto, but skipping the loop
				// termination check and increment caused all kinds of corner-
				// case bugs.  I think this is more robust.
				region_i_empty = true;
				break;
			}
		}
		
		if (!region_i_empty) {
			// Moved region that are now dirty don't need to be moved!
			const BRegion new_dirty = first.m_movedRegions[i].region & (next.m_dirtyRegion - offset);
			if (new_dirty.IsEmpty() == false) {
				first.m_movedRegions.EditItemAt(i).region -= new_dirty;
				if (first.m_movedRegions[i].region.IsEmpty()) {
					first.m_movedRegions.RemoveItemsAt(i);
					count--; i--;
				}
			}
		}
	}

	// Finally, what stays, should be good...
	for (int32 i=0 ; i<first.CountRegions() ; i++)
		result.add_record(first.m_movedRegions[i]);
	for (int32 i=0 ; i<next.CountRegions() ; i++)
		result.add_record(next.m_movedRegions[i]);

	// update the remaining regions
	result.m_cleanRegion = first.m_cleanRegion | next.m_cleanRegion;
	result.m_newRegion = first.m_newRegion | next.m_newRegion;
	result.m_translucent = (first.m_translucent - next.m_cleanRegion) | next.m_translucent;
	return result;
}



////////////////////////////////////////////////////////
// #pragma mark -


void BUpdate::Transform(const B2dTransform& t)
{
//REVISIT: we might have to make sure the transformation isn't too complex
// because the copy-blit could be imprecise.
	// Apply the transformation
	for (int32 i=0 ; i<CountRegions() ; i++) {
		m_movedRegions.EditItemAt(i).region.Transform(t);
		m_movedRegions.EditItemAt(i).offset = t.DeltaTransform(m_movedRegions[i].offset);
	}
	m_dirtyRegion.Transform(t);
	m_newRegion.Transform(t);
	m_cleanRegion.Transform(t);
	m_translucent.Transform(t);
}

void BUpdate::add_record(const record& r)
{
	const uint32 count = CountRegions();
	for (uint32 i=0 ; i<count ; i++) {
		if (r.offset == m_movedRegions[i].offset) {
			m_movedRegions.EditItemAt(i).region |= r.region;
			return;
		}
	}
	m_movedRegions.AddItem(r);
}

////////////////////////////////////////////////////////
// #pragma mark -
#warning Un/Flatten: is not endian safe

BValue BUpdate::AsValue() const
{
	return BValue::Flat(*this);
}
bool BUpdate::IsFixedSize() const {
	return false;
}
type_code BUpdate::TypeCode() const {
	return B_UPDATE_TYPE;
}
bool BUpdate::AllowsTypeCode(type_code code) const {
	return code == B_UPDATE_TYPE;
}

ssize_t BUpdate::FlattenedSize() const
{
	ssize_t size = 0;
	size += sizeof(reg_sizes_t);
	size += m_cleanRegion.FlattenedSize();
	size += m_translucent.FlattenedSize();
	size += m_dirtyRegion.FlattenedSize();
	size += m_newRegion.FlattenedSize();
	size += m_srcTransform.FlattenedSize();
	size += m_dstTransform.FlattenedSize();
	for (int32 i=0 ; i<CountRegions() ; i++)
		size += (sizeof(ssize_t) + sizeof(m_movedRegions[i].offset) + m_movedRegions[i].region.FlattenedSize());
	return size;
}

status_t BUpdate::Flatten(void *buffer, ssize_t size) const
{
	if (size < FlattenedSize())
		return B_BAD_VALUE;
	const ssize_t transform2Dsize = m_srcTransform.FlattenedSize();
	reg_sizes_t& rsize = *static_cast<reg_sizes_t *>(buffer);
	rsize.clean = m_cleanRegion.FlattenedSize();
	rsize.trans = m_translucent.FlattenedSize();
	rsize.dirty = m_dirtyRegion.FlattenedSize();
	rsize.nshap = m_newRegion.FlattenedSize();
	buffer = static_cast<char *>(buffer) + sizeof(reg_sizes_t); size -= sizeof(reg_sizes_t);
	m_srcTransform.Flatten(buffer, transform2Dsize); size -= transform2Dsize; buffer = static_cast<char *>(buffer) + transform2Dsize;
	m_dstTransform.Flatten(buffer, transform2Dsize); size -= transform2Dsize; buffer = static_cast<char *>(buffer) + transform2Dsize;
	m_cleanRegion.Flatten(buffer, rsize.clean);	size -= rsize.clean; buffer = static_cast<char *>(buffer) + rsize.clean;
	m_translucent.Flatten(buffer, rsize.trans);	size -= rsize.trans; buffer = static_cast<char *>(buffer) + rsize.trans;
	m_dirtyRegion.Flatten(buffer, rsize.dirty);	size -= rsize.dirty; buffer = static_cast<char *>(buffer) + rsize.dirty;
	  m_newRegion.Flatten(buffer, rsize.nshap);	size -= rsize.nshap; buffer = static_cast<char *>(buffer) + rsize.nshap;
	for (int32 i=0 ; i<CountRegions() ; i++) {
		if (size <= 0)
			return B_ERROR;
		ssize_t s = m_movedRegions[i].region.FlattenedSize();
		*static_cast<BPoint *>(buffer) = m_movedRegions[i].offset;	buffer = static_cast<char *>(buffer) + sizeof(BPoint);
		*static_cast<ssize_t *>(buffer) = s;						buffer = static_cast<char *>(buffer) + sizeof(ssize_t);
		m_movedRegions[i].region.Flatten(buffer, s);				buffer = static_cast<char *>(buffer) + s;
		size -= (s + sizeof(BPoint) + sizeof(ssize_t));
	}
	return B_OK;
}


status_t BUpdate::Unflatten(type_code c, const void *buf, ssize_t size)
{
	if (c != B_UPDATE_TYPE)
		return B_BAD_TYPE;
	const ssize_t transform2Dsize = m_srcTransform.FlattenedSize();
	const reg_sizes_t& rsize = *static_cast<const reg_sizes_t *>(buf);
	buf = static_cast<const char *>(buf) + sizeof(reg_sizes_t); size -= sizeof(reg_sizes_t);
	m_srcTransform.Unflatten(B_REGION_TYPE, buf, transform2Dsize); size -= transform2Dsize; buf = static_cast<const char *>(buf) + transform2Dsize;
	m_dstTransform.Unflatten(B_REGION_TYPE, buf, transform2Dsize); size -= transform2Dsize; buf = static_cast<const char *>(buf) + transform2Dsize;
	m_cleanRegion.Unflatten(B_REGION_TYPE, buf, rsize.clean); size -= rsize.clean; buf = static_cast<const char *>(buf) + rsize.clean;
	m_translucent.Unflatten(B_REGION_TYPE, buf, rsize.trans); size -= rsize.trans; buf = static_cast<const char *>(buf) + rsize.trans;
	m_dirtyRegion.Unflatten(B_REGION_TYPE, buf, rsize.dirty); size -= rsize.dirty; buf = static_cast<const char *>(buf) + rsize.dirty;
	  m_newRegion.Unflatten(B_REGION_TYPE, buf, rsize.nshap); size -= rsize.nshap; buf = static_cast<const char *>(buf) + rsize.nshap;
	for (int32 i=0 ; i<CountRegions() ; i++) {
		if (size <= 0)
			return B_BAD_VALUE;
		record rec;
		rec.offset = *static_cast<const BPoint *>(buf);			buf = static_cast<const char *>(buf) + sizeof(BPoint);
		const ssize_t& s = *static_cast<const ssize_t *>(buf);	buf = static_cast<const char *>(buf) + sizeof(ssize_t);
		rec.region.Unflatten(B_REGION_TYPE, buf, s);			buf = static_cast<const char *>(buf) + s;
		m_movedRegions.AddItem(rec);
		size -= (s + sizeof(BPoint) + sizeof(ssize_t));
	}
	return B_OK;
}

} } // namespace B::Interface2

