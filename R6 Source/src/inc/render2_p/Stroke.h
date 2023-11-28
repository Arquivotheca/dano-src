
//******************************************************************************
//
//	File:			Stroke.h
//	Description:	Utility class to generate thick Postscript-style lines
//	Written by:		George Hoffman
//
//	Copyright 2001, Be Incorporated
//
//******************************************************************************/

#ifndef	RENDER2_STROKE_H
#define	RENDER2_STROKE_H

#include <support2/Vector.h>
#include <render2/RenderDefs.h>
#include <render2/Path.h>

namespace B {
namespace Render2 {

class BStroker : public IPath {

	public:
		
								BStroker(	IPath::arg destination,
											float penSize=1.0,
											cap_mode cap=B_BUTT_CAP, join_mode join=B_BEVEL_JOIN,
											float miter=10.0);
								~BStroker();
		
				void			LineDependencies();
				void			Stroke(bool close);

				void			SetPenSize(float size);
				void			SetCapping(cap_mode capping);
				void			SetJoining(join_mode joining);
				void			SetMiter(float miter);
		
		virtual	void			MoveTo(const BPoint& pt);
		virtual	void			LinesTo(const BPoint* points, int32 lineCount=1);
		virtual	void			Close();

	private:

		typedef void 			(*LineCapFunc)(
									BStroker *thickener,
									BPoint p, BPoint delta);
				
		typedef void 			(*LineJoinFunc)(
									BStroker *thickener,
									BPoint corner, BPoint delta1, BPoint delta2);

				IPath::ptr		m_destination;
				BVector<BPoint>	m_cache;

				float			m_penSize;
				cap_mode		m_cap;
				join_mode		m_join;
				float			m_miter;

				LineCapFunc		m_capFunc;
				LineCapFunc		m_internalCapFunc;
				LineJoinFunc	m_joinFunc;
				
				float			m_theta;
				int32			m_numThetaSteps;
				int32			m_thetaStepsHighWater;
				float *			m_cosThetaSteps;
				float *			m_sinThetaSteps;
				float			m_cosMiterLimit;
				float			m_halfPen;
				float			m_penSize2_;
				
		static	LineCapFunc		cap_funcs[5];
		static	LineJoinFunc	join_funcs[5];
		static	void 			GenerateRoundCap(BStroker *shape, BPoint p, BPoint delta);
		static	void 			GenerateSquareCap(BStroker *shape, BPoint p, BPoint delta);
		static	void 			GenerateButtCap(BStroker *shape, BPoint p, BPoint delta);
		static	void 			GenerateRoundCorner(BStroker *shape, BPoint corner, BPoint delta1, BPoint delta2);
		static	void 			GenerateMiterCorner(BStroker *shape, BPoint corner, BPoint delta1, BPoint delta2);
		static	void 			GenerateBevelCorner(BStroker *shape, BPoint corner, BPoint delta1, BPoint delta2);
};

} }

#endif
