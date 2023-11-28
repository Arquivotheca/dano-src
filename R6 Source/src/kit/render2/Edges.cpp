
#include <support2/StdIO.h>
#include <render2_p/Edges.h>

namespace B {
namespace Render2 {

void 
BEdges::MoveTo(const BPoint &pt)
{
	bout << "MoveTo(" << pt << ")" << endl;
}

void 
BEdges::LinesTo(const BPoint *points, int32 lineCount)
{
	while (lineCount--)
		bout << "LineTo(" << *points++ << ")" << endl;
}

void 
BEdges::Close()
{
	bout << "Close()" << endl;
}

} }
