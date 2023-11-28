#ifndef QUADNODE_H
#define QUADNODE_H

#include "graphic.h"


class BGQuadNode : public BMGraphicGroup
{
public:
			BGQuadNode(const char *name, const BRect frame, const int32 granularity=20, const int32 capacity=10);
			//BGQuadNode(const BQuadNode &rhs);
			~BGQuadNode();
			
	virtual void	Draw(BGraphPort *, const BRect );
	virtual void	DrawFrame(BGraphPort *, const BRect);
	virtual void	MoveTo(const float x, const float y);
	
	// Interface for adding graphics
	virtual	void	AddChild(BMGraphic *);
	virtual	void	AddChildren(List<BMGraphic *> *aList);
	virtual void	Remove(BMGraphic *);
	virtual int32	CountChildren();
	
	// Returns whether a point is contained in a quadrant
	virtual bool		Contains(const BPoint where);
	virtual BMGraphic	* GraphicAt(const BPoint);
	virtual void		Optimize();
	
protected:
	int32	fGranularity;
	int32	fCapacity;
	bool	fIsDivided;
	BRect	fArea;
	
	BGQuadNode	*fQuadrants[4];
	
private:
	BGQuadNode(const BGQuadNode &);
	BGQuadNode & operator = (const BGQuadNode &);
};


#endif
