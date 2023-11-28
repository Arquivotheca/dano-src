/***************************************************************************
//
//	File:			media2/MediaPreference.h
//
//	Description:	Stores a set of weighted key -> preferred value mappings
//					to aid in the translation from a format "wildcard"
//					(BMediaConstraint) to a fully-specified format
//					(BMediaFormat.)
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIAPREFERENCE_H_
#define _MEDIA2_MEDIAPREFERENCE_H_

#include <support2/Value.h>

namespace B {
namespace Media2 {

using namespace Support2;

class BMediaPreference
{
public:
						BMediaPreference();
						BMediaPreference(const BValue & value);
	virtual				~BMediaPreference();

			status_t	AddItem(
							const BValue & key,
							const BValue & value, float weight = 1.0f);
			
			status_t	GetNextKey(
							void ** cookie,	BValue * outKey) const;

			status_t	GetNextItem(
							const BValue & key, void ** cookie,
							BValue * outValue, float * outWeight) const;
			
			status_t	RemoveKey(const BValue & key);
			status_t	RemoveItem(const BValue & key, const BValue & value);
			
			status_t	Overlay(const BMediaPreference & prefs);
			
			void		Undefine();
			bool		IsDefined() const;
	inline				operator bool() const { return IsDefined(); }
		
			BValue		AsValue() const;
	inline				operator BValue() const { return AsValue(); }

private:
			// (key -> (value -> weight))
			BValue		mData;
};

} } // B::Media2
#endif //_MEDIA2_MEDIAPREFERENCE_H_
