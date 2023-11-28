/*******************************************************************************
/
/	File:			Shape.cpp
/
/   Description:    BShape encapsulates a Postscript-style "path"
/
/	Copyright 1992-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#include <Shape.h>

#include <Debug.h>
#include <Point.h>
#include <Message.h>
#include <Transform2d.h>
#include <StreamIO.h>

#include <interface_misc.h>
#include <shared_support.h>

#include <new>

class shape_data {
	public:
	BArray<uint32>	ops;
	BArray<BPoint>	points;
};

#define	MAX_OUT		0x7ffffffd

#define PATH_MOVETO		0x80000000
#define PATH_CLOSE		0x40000000
#define PATH_LINE		0x10000000
#define PATH_BEZIER		0x20000000
#define PATH_OPCODE		0x30000000
#define PATH_OPBITS		0xF0000000
#define PATH_COUNT		0x0FFFFFFF

/*----------------------------------------------------------------*/

void BShapeIterator::_ReservedShapeIterator1() {};
void BShapeIterator::_ReservedShapeIterator2() {};
void BShapeIterator::_ReservedShapeIterator3() {};
void BShapeIterator::_ReservedShapeIterator4() {};

BShapeIterator::BShapeIterator()
{
};

BShapeIterator::~BShapeIterator()
{
};

status_t BShapeIterator::IterateMoveTo(BPoint */*point*/)
{
	return B_OK;
};

status_t BShapeIterator::IterateLineTo(int32 /*lineCount*/, BPoint */*linePts*/)
{
	return B_OK;
};

status_t BShapeIterator::IterateBezierTo(int32 /*bezierCount*/, BPoint */*bezierPts*/)
{
	return B_OK;
};

status_t BShapeIterator::IterateClose()
{
	return B_OK;
};

status_t BShapeIterator::Iterate(const BShape *shape)
{
	int32 opCount,ptCount,count;
	uint32 *opList;
	BPoint *ptList;
	uint32 op;
	status_t r;

	IKAccess::GetShapeData(shape,&opCount,&ptCount,&opList,&ptList);
	for (int32 opIndex=0;opIndex<(opCount+1);opIndex++) {
		if (opIndex == opCount) op = IKAccess::ShapeBuildingOp(shape);
		else					op = opList[opIndex];
		count = op & PATH_COUNT;

		if (op & PATH_MOVETO) {
			if (opIndex && (op & PATH_CLOSE)) if ((r=IterateClose()) != B_OK) return r;
			if ((r=IterateMoveTo(ptList++)) != B_OK) return r;
		};

		switch (op & PATH_OPCODE) {
			case PATH_LINE:
				if ((r=IterateLineTo(count,ptList)) != B_OK) return r;
				ptList += count;
				break;
			case PATH_BEZIER:
				if ((r=IterateBezierTo(count/3,ptList)) != B_OK) return r;
				ptList += count;
				break;
			default:
				if (op & PATH_CLOSE) {
					if ((r=IterateClose()) != B_OK) return r;
				};
				return B_OK;
		};
	};
	return B_OK;
};

/*----------------------------------------------------------------*/

void BShape::_ReservedShape1() {};
void BShape::_ReservedShape2() {};
void BShape::_ReservedShape3() {};
void BShape::_ReservedShape4() {};

BShape::BShape()
{
	InitData();
}

BShape::BShape(const BShape &copyFrom)
	:	BArchivable()
{
	InitData();
	AddShape(&copyFrom);
}

BShape::BShape(const BShape &copyFrom, const BTransform2d& t)
	:	BArchivable()
{
	InitData();
	AddShape(&copyFrom, t);
}

BShape::BShape(BMessage *msg)
{
	InitData();
	shape_data *data = (shape_data*)fPrivateData;
	
	int32 ptCount,opCount,*ops;
	BPoint *pts;
	for (int32 i=0;(msg->FindData("pts",B_POINT_TYPE,i,(const void**)&pts,&ptCount) == B_OK);i++)
		data->points.AddItem(*pts);
	for (int32 i=0;(msg->FindData("ops",B_INT32_TYPE,i,(const void**)&ops,&opCount) == B_OK);i++)
		data->ops.AddItem(*ops);
	fBuildingOp = data->ops[data->ops.CountItems()-1];
	data->ops.SetItems(data->ops.CountItems()-1);
}

BShape::~BShape()
{
	shape_data *data = (shape_data*)fPrivateData;
	delete data;
};

BShape& BShape::operator=(const BShape& o)
{
	if (this != &o) {
		Clear();
		AddShape(&o);
	}
	return *this;
}

status_t BShape::Archive(BMessage *into, bool /*deep*/) const
{
	shape_data *data = (shape_data*)fPrivateData;
	if (!data->points.CountItems() || !data->ops.CountItems()) return B_OK;

	into->AddData("pts",B_POINT_TYPE,
		&data->points[0],sizeof(BPoint),
		true,data->points.CountItems());
	for (int32 i=1;i<data->points.CountItems();i++)
		into->AddPoint("pts",data->points[i]);

	into->AddData("ops",B_INT32_TYPE,
		&data->ops[0],sizeof(int32),
		true,data->ops.CountItems()+1);
	for (int32 i=1;i<data->ops.CountItems();i++)
		into->AddInt32("ops",data->ops[i]);
	into->AddInt32("ops",fBuildingOp);

	return B_OK;
};

BArchivable *BShape::Instantiate(BMessage *data)
{
	return new BShape(data);
};

status_t BShape::Perform(perform_code /*d*/, void */*arg*/)
{
	ASSERT(!"not implemented");
	return B_OK;
};

void BShape::Clear()
{
	fBuildingOp = 0;
	shape_data *data = (shape_data*)fPrivateData;
	data->points.SetItems(0);
	data->ops.SetItems(0);
};

BRect BShape::Bounds() const
{
	shape_data *data = (shape_data*)fPrivateData;
	if (!data->points.CountItems()) return BRect();

	BPoint *pt = data->points.Items();
	BRect bounds(pt->x,pt->y,pt->x,pt->y);
	pt++;

	for (int32 count = data->points.CountItems()-1;count;count--,pt++) {
		if (pt->x > bounds.right) bounds.right = pt->x;
		if (pt->x < bounds.left) bounds.left = pt->x;
		if (pt->y > bounds.bottom) bounds.bottom = pt->y;
		if (pt->y < bounds.top) bounds.top = pt->y;
	};

	return bounds;
};

status_t BShape::MoveTo(BPoint point)
{
	shape_data *data = (shape_data*)fPrivateData;
	if (fBuildingOp & PATH_OPCODE) {
		data->ops.AddItem(fBuildingOp);
		data->points.AddItem(point);
		fBuildingOp = PATH_MOVETO;
	} else if (fBuildingOp & PATH_MOVETO) {
		data->points[data->points.CountItems()-1] = point;
	} else {
		data->points.AddItem(point);
		fBuildingOp |= PATH_MOVETO;
	};
	return B_OK;
};

status_t BShape::LineTo(BPoint linePoint)
{
	shape_data *data = (shape_data*)fPrivateData;
	uint32 op = (fBuildingOp & PATH_OPCODE);
	if (op && (op != PATH_LINE)) {
		data->ops.AddItem(fBuildingOp);
		fBuildingOp = 0;
	};
	fBuildingOp |= PATH_LINE;
	data->points.AddItem(linePoint);
	fBuildingOp++;
	return B_OK;
};

status_t BShape::BezierTo(BPoint controlPoints[3])
{
	shape_data *data = (shape_data*)fPrivateData;
	uint32 op = (fBuildingOp & PATH_OPCODE);
	if (op && (op != PATH_BEZIER)) {
		data->ops.AddItem(fBuildingOp);
		fBuildingOp = 0;
	};
	fBuildingOp |= PATH_BEZIER;
	for (int32 i=0;i<3;i++)
		data->points.AddItem(controlPoints[i]);
	fBuildingOp+=3;
	return B_OK;
};

status_t BShape::Close()
{
	shape_data *data = (shape_data*)fPrivateData;
	if (fBuildingOp & PATH_OPCODE) {
		data->ops.AddItem(fBuildingOp);
		fBuildingOp = PATH_CLOSE;
	};
	return B_OK;
};

void BShape::ApplyTransform(const BTransform2d& t)
{
	int32 opCount, ptCount;
	uint32* opList;
	BPoint* ptList;
	GetData(&opCount, &ptCount, &opList, &ptList);
	t.Transform(ptList, ptCount);
}

BShape& BShape::operator*=(const BTransform2d& t)
{
	ApplyTransform(t);
	return *this;
}

BShape BShape::operator*(const BTransform2d& t) const
{
	return BShape(*this, t);
}

void BShape::InitData()
{
	fPrivateData = new shape_data();
	Clear();
};

status_t BShape::AddShape(const BShape *other)
{
	shape_data *data = (shape_data*)fPrivateData;
	shape_data *otherData = (shape_data*)other->fPrivateData;
	int32 opCount = data->ops.CountItems();
	data->ops.AddArray(&otherData->ops);
	data->points.AddArray(&otherData->points);
	if (data->ops.CountItems() > opCount) {
		data->ops[opCount] |= fBuildingOp;
		fBuildingOp = other->fBuildingOp;
	};
	return B_OK;
};

status_t BShape::AddShape(const BShape *other, const BTransform2d& t)
{
	shape_data *data = (shape_data*)fPrivateData;
	shape_data *otherData = (shape_data*)other->fPrivateData;
	int32 opCount = data->ops.CountItems();
	data->ops.AddArray(&otherData->ops);
	int32 ptCount = data->points.CountItems();
	data->points.SetItems(ptCount+otherData->points.CountItems());
	t.Transform(data->points.Items()+ptCount, otherData->points.Items(),
				otherData->points.CountItems());
	if (data->ops.CountItems() > opCount) {
		data->ops[opCount] |= fBuildingOp;
		fBuildingOp = other->fBuildingOp;
	};
	return B_OK;
};

void BShape::GetData(int32 *opCount, int32 *ptCount, uint32 **opList, BPoint **ptList) const
{
	shape_data *data = (shape_data*)fPrivateData;
	*opCount = data->ops.CountItems();
	*ptCount = data->points.CountItems();
	*opList = data->ops.Items();
	*ptList = data->points.Items();
};

void BShape::SetData(int32 opCount, int32 ptCount, uint32 *opList, BPoint *ptList)
{
	shape_data *data = (shape_data*)fPrivateData;
	//opCount--;
	fBuildingOp = opList[opCount] & PATH_CLOSE;
//	if(fBuildingOp & 0x30000000 == 0x30000000) {
//		fBuildingOp &= ~0x30000000;
//	}
	data->ops.SetItems(opCount);
	data->points.SetItems(ptCount);
	while (opCount--) data->ops[opCount] = opList[opCount];
	while (ptCount--) data->points[ptCount] = ptList[ptCount];
};

void BPrivate::IKAccess::GetShapeData(const BShape* shape,
							 int32 *opCount, int32 *ptCount,
							 uint32 **opList, BPoint **ptList)
{
	shape->GetData(opCount, ptCount, opList, ptList);
}

uint32 BPrivate::IKAccess::ShapeBuildingOp(const BShape* shape)
{
	return shape->fBuildingOp;
}

/*----------------------------------------------------------------*/

void	BShape::PrintToStream() const
{
#if SUPPORTS_STREAM_IO
	BOut << *this << endl;
#endif
}

/*----------------------------------------------------------------*/

#if SUPPORTS_STREAM_IO
namespace BPrivate {
	class StreamShapeIterator : public BShapeIterator {
	public:
		StreamShapeIterator(BDataIO& io)	: mIO(io)	{ }
		~StreamShapeIterator()							{ }
		
		status_t IterateMoveTo(BPoint *point)							{ mIO << "\n\t MoveTo " << *point; return B_OK; }
		status_t IterateLineTo(int32 lineCount, BPoint *linePts)		{ mIO << "\n\t LineTo"; while (lineCount--) mIO << " " << *(linePts++); return B_OK; }
		status_t IterateBezierTo(int32 bezierCount, BPoint *bezierPts)	{ mIO << "\n\t BezierTo"; while (bezierCount--) mIO << " " << *(bezierPts++); return B_OK; }
		status_t IterateClose()											{ mIO << "\n\t Close"; return B_OK; }
	private:
		BDataIO& mIO;
	};
}
using namespace BPrivate;
#endif

BDataIO& operator<<(BDataIO& io, const BShape& shape)
{
#if SUPPORTS_STREAM_IO
	io << "BShape(" << shape.Bounds() << ") {";
	StreamShapeIterator ssi(io);
	ssi.Iterate(&shape);
	io << "\n}";
#else
	(void)shape;
#endif

	return io;
}

// --------- Deprecated BShape methods 11/1999 (Maui) ---------

#if _R4_5_COMPATIBLE_
extern "C" {

	_EXPORT BShape*
	#if __GNUC__
	__6BShapeR6BShape
	#elif __MWERKS__
	__ct__6BShapeFR6BShape
	#endif
	(void* This, BShape& o)
	{
		return new (This) BShape(o);
	}
	
	_EXPORT status_t
	#if __GNUC__
	AddShape__6BShapeP6BShape
	#elif __MWERKS__
	AddShape__6BShapeFP6BShape
	#endif
	(BShape* This, BShape *other)
	{
		return This->AddShape(other);
	}
	
	_EXPORT BRect
	#if __GNUC__
	Bounds__6BShape
	#elif __MWERKS__
	Bounds__6BShapeFv
	#endif
	(BShape* This)
	{
		return This->Bounds();
	}

}
#endif

// --------- Deprecated BShape methods 08/2000 (Dano?) ---------

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT status_t
	#if __GNUC__
	Iterate__14BShapeIteratorP6BShape
	#elif __MWERKS__
	Iterate__14BShapeIteratorFP6BShape
	#endif
	(BShapeIterator* This, BShape *shape)
	{
		return This->Iterate(shape);
	}
	
}
#endif

