

#include	<termio.h>
#include	<Looper.h>
#include	"RNode.h"
#include	<math.h>
#include	<string.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	"rico.h"
#include	"RNodeHandler.h"
#include	"XString.h"
#include	"Termio.h"

#include	<Message.h>


BTermio::BTermio( RNode n1, RNode n2, RNode n3, RNode n4, RNode n5, RNode n6):
din( this, n1), dout( this, n2), rin( this, n3), rout( this, n4),
cin( n5), cout( this, n6)
{

}


BTermio::DataIn::DataIn( BTermio *t, RNode n):
termio( t), RNodeHandler( n)
{

	full = FALSE;
	stopped = FALSE;
	node.Establish( this, "BTermio::DataIn");
}


void
BTermio::DataIn::MessageReceived( BMessage *m)
{
	uchar	*p;
	long	n;

	//printf( "BTermio::DataIn::MessageReceived: what=%d\n", m->what);
	switch (m->what) {
	case EOF:
		node.Shutdown( );
		break;
	case 'sig2':
		if (full) {
			x.clear( );
			full = FALSE;
		}
		node >> 'sig2';
		break;
	default:
		if (m->FindData( "", B_UINT8_TYPE, (const void **)&p, &n) == B_OK) {
			uint i = 0;
			while (i < n)
				x.putb( p[i++]);
			full = TRUE;
			process( );
		}
	}
}


void
BTermio::DataIn::process( )
{
	uint	n;

	if ((n = x.count( ))
	and (not stopped)) {
		BMessage *m = new BMessage;
		m->AddData( "", B_UINT8_TYPE, x.getm( n), n);
		node >> m;
		stopped = TRUE;
		if (full)
			full = FALSE;
	}
}


BTermio::DataOut::DataOut( BTermio *t, RNode n):
termio( t), RNodeHandler( n)
{

	full = FALSE;
	stopped = FALSE;
	node.Establish( this, "BTermio::DataOut");
}


void
BTermio::DataOut::MessageReceived( BMessage *m)
{
	uchar	*p;
	long	n;

	//printf( "BTermio::DataOut::MessageReceived: what=%d\n", m->what);
	if (m->what == EOF)
		node.Shutdown( );
	else {
		m->FindData( "", B_UINT8_TYPE, (const void **)&p, &n);
		if (p) {
			uint i = 0;
			while (i < n)
				x.putb( p[i++]);
		}
		full = TRUE;
		process( );
	}
}


void
BTermio::DataOut::process( )
{

	unless (stopped) {
		if (full) {
			termio->rin.node >> 0;
			full = FALSE;
		}
		if (uint n = x.count( )) {
			BMessage *m = new BMessage( );
			m->AddData( "", B_UINT8_TYPE, x.getm( n), n);
			node >> m;
			stopped = TRUE;
		}
	}
}


BTermio::ReqIn::ReqIn( BTermio *t, RNode n):
termio( t), RNodeHandler( n)
{

	node.Establish( this, "BTermio::ReqIn");
}


void
BTermio::ReqIn::MessageReceived( BMessage *m)
{

	if (m->what == EOF)
		node.Shutdown( );
	else {
		termio->dout.stopped = FALSE;
		termio->dout.process( );
	}
}


BTermio::ReqOut::ReqOut( BTermio *t, RNode n):
termio( t), RNodeHandler( n)
{

	node.Establish( this, "BTermio::ReqOut");
}


void
BTermio::ReqOut::MessageReceived( BMessage *m)
{

	if (m->what == EOF)
		node.Shutdown( );
	else {
		termio->din.stopped = FALSE;
		termio->din.process( );
	}
}


BTermio::CtlIn::CtlIn( RNode n):
RNodeHandler( n)
{

	node.Establish( this, "BTermio::CtlIn");
}


void
BTermio::CtlIn::MessageReceived( BMessage *m)
{
	char	*s;
	long	n;

	n = m->FlattenedSize();
	s = (char*) malloc(n);
	m->Flatten( s, n);
	BMessage *newm = new BMessage;
	newm->Unflatten( s);
	free( s);
	node >> newm;
}


BTermio::CtlOut::CtlOut( BTermio *tio, RNode n):
termio( tio), RNodeHandler( n)
{

	node.Establish( this, "BTermio::CtlOut");
}


void
BTermio::CtlOut::MessageReceived( BMessage *m)
{
	long	n;

	switch (m->what) {
	case EOF:
		node.Shutdown( );
		break;
	case 'Gr&c':
		node >> 'Gr&c';
		break;
	case 'pref':
		char *s;
		long n;
		n = m->FlattenedSize();
		s = (char*) malloc(n);
		m->Flatten( s, n);
		m = new BMessage;
		m->Unflatten( s);
		free( s);
		node >> m;
	}
}
