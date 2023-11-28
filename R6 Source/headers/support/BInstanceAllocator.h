/***************************************************************************
//
//	File:			support/BInstanceAllocator.h
//
//	Description:	Base class for a class hierarchy, which provides more
//					optimimal heap allocation of instance data.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT_INSTANCE_ALLOCATOR_H
#define _SUPPORT_INSTANCE_ALLOCATOR_H

#include <support/SupportDefs.h>

namespace B {
namespace Support {

class BInstanceAllocator
{
public:
virtual					~BInstanceAllocator();

protected:
						BInstanceAllocator(size_t total_instance_size=0);

		// Use this function when calling up to your superclass's
		// constructor, to add your instance data memory needs into
		// the hierarchy.  The final BInstanceAllocator constructor
		// will be called with the total instance data required.
static	size_t			AddInstanceData(size_t current, size_t additional);
		
		// Call this function in your constructor implementation to
		// retrieve the instance data for your class.  You will need
		// to manually call in-place constructors and destructors for
		// any objects in your instance data.
		void*			GetInstanceData(size_t amount);
private:
		struct instance_data {
			size_t amount;
			size_t pos;
		};
		
		instance_data*	fInstanceData;
};

} }	// namespace B::Support

#endif
