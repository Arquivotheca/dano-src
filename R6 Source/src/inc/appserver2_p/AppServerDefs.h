
#ifndef	_APPSERVER2_DEFS_H_
#define	_APPSERVER2_DEFS_H_

#include <support2/Atom.h>

namespace B {
namespace Raster2 { }
namespace Render2 { }
namespace Interface2 { }
namespace AppServer2 {

using namespace Support2;
using namespace Raster2;
using namespace Render2;
using namespace Interface2;

class BAppServer;
class BAppServerBitmap;
class BAppServerSurface;
class BAppServerRootWindow;
class BAppServerHostedSurface;
class BAppServerEventPort;
class BAppServerRenderer;

typedef atom_ptr<BAppServerSurface> surface;
typedef atom_ref<BAppServerSurface> surface_ref;
typedef atom_ptr<BAppServerHostedSurface> hostedsurface;

} }	// namespace B::AppServer2

using namespace B::AppServer2;

#endif 	/* _APPSERVER2_DEFS_H_ */
