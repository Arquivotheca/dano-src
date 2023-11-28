// a local-access x86 nub is simply a DLocalNub with the Dx86Nub mixin

#ifndef DLOCALX86NUB_H
#define DLOCALX86NUB_H 1

#include "DLocalNub.h"
#include "Dx86Nub.h"
#include <OS.h>

class DTeam;

class DLocalx86Nub : virtual public DLocalNub, virtual public Dx86Nub
{
public:
	DLocalx86Nub(port_id, port_id, DTeam*);
};

#endif
