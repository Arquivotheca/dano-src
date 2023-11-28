#ifndef __DIALER_H
#define __DIALER_H

#include <support/Atom.h>
#include <support/Binder.h>

class Dialer
{
public:
					Dialer();

	bool			CanAutodial();

private:
	atom<BinderListener>	m_autodialListener;
};

#endif	/* __DIALER_H */
