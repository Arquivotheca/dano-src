/***************************************************************************
//
//	File:			render2/IPath.h
//
//	Description:	Path definition interface
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _RENDER2_EDGES_H_
#define _RENDER2_EDGES_H_

#include <math.h>
#include <support2/SupportDefs.h>
#include <support2/IInterface.h>
#include <render2/Path.h>

namespace B {
namespace Render2 {

/**************************************************************************************/

class BEdges : public IPath
{
	public:

		virtual	void				MoveTo(const BPoint& pt);
		virtual	void				LinesTo(const BPoint* points, int32 lineCount=1);
		virtual	void				Close();

	private:
	
				void *				m_imp;
};

/**************************************************************************************/

} } // namespace B::Render2

#endif
