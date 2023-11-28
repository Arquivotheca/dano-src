
#ifndef _B_XML_EXPRESSIBLE_H
#define _B_XML_EXPRESSIBLE_H

#include <Atom.h>

#include <xml/BParser.h>
#include <xml/BWriter.h>

namespace B {
namespace XML {

class BExpressible : virtual public BAtom
{
public:
									BExpressible();
	virtual							~BExpressible();
	
	virtual status_t				Code(BCodifier *stream);
	virtual status_t				Parse(BParser **stream);
};

}; // namespace XML
}; // namespace B

#endif  // _B_XML_EXPRESSIBLE_H
