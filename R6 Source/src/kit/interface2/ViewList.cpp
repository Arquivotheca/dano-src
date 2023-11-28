
#include <interface2/View.h>
#include <interface2/ViewList.h>

using namespace B::Support2;
using namespace B::Interface2;

#define IMPLEMENT_INTERFACE		1
#define IMPLEMENT_REMOTE		1
#define IMPLEMENT_CONCRETE		1
#define IMPLEMENT_BASE			1
#define CHILD_TYPE				IView
#define THIS_TYPE				typeid(IViewList)

#include <support2/ListImplementation.h>
