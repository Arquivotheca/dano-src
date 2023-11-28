
#include "ObjectView.h"

struct point {
	float nx,ny,nz;
	float x,y,z;
};

struct tri {
	int p1,p2,p3;
};

struct quadStrip {
	int numpts;
	int *pts;
};

#include "util.h"

class GLObject {
public:
float 				rotX,rotY,spinX,spinY,lastRotX,lastRotY;
float				x,y,z;
int					color;
int					solidity;
bool				changed;
ObjectView *		objView;

					GLObject(ObjectView *ov);
virtual				~GLObject();

virtual void		Draw(bool forID, float IDcolor[]);
virtual bool		SpinIt();
virtual void		MenuInvoked(BPoint point);

virtual void		DoDrawing(bool forID) {};
};

class TriangleObject : public GLObject {
public:
point *vertexArrayData;
BufferArray<tri>		triangles;
BufferArray<quadStrip>	qs;

					TriangleObject(ObjectView *ov, char *filename);
virtual				~TriangleObject();

virtual void		DoDrawing(bool forID);
};
