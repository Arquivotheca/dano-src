/***************************************************************************
//
//	File:			support2/Root.h
//
//	Description:	Top level interface to a component.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT2_ROOT_H_
#define _SUPPORT2_ROOT_H_

#include <support2/IRoot.h>
#include <support2/Atom.h>
#include <support2/Binder.h>
#include <support2/IBinder.h>
#include <image.h>

namespace B {
namespace Support2 {


/*--------------------------------------------------------*/
/*----- BImage -------------------------------------------*/
class BImage : public virtual BAtom
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BImage);
	
						BImage(image_id id);
protected:
	virtual				~BImage();

private:
			image_id	m_image;
};


/*--------------------------------------------------------*/
/*----- BImageRef ----------------------------------------*/
/*!	All of your exported classes should inherit from this, so that when
	all references to it are deleted, your add-on will be unloaded.
*/
class BImageRef
{
public:
					BImageRef();
					~BImageRef();
	
	BImage::ptr		Image() const;
		
private:
	BImage::ptr		m_image;
};



/*--------------------------------------------------------*/
/*----- LRoot / BRoot ------------------------------------*/

class LRoot : public LInterface<IRoot>
{
public:
	virtual	status_t Called(BValue &in, const BValue &outBindings, BValue &out);
};

class BRoot : public LRoot
{
public:
	BRoot();
	virtual ~BRoot();
	
	virtual IBinder::ptr	LoadObject(BString pathname, const BValue & params);
	
private:
	explicit BRoot(BRoot & copy);
};


} } // namespace B::Support2

#endif	/* _SUPPORT2_ROOT_H_ */
