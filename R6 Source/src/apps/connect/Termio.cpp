

#include	<termio.h>
#include	<Looper.h>
#include	<Message.h>
#include	"RNode.h"
#include	<string.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	"rico.h"
#include	"RNodeHandler.h"
#include	"XString.h"
#include	"Termio.h"


BTermio::BTermio( RNode n1, RNode n2, RNode n3, RNode n4, RNode n5, RNode n6):
din( this, n1), dout( this, n2), rin( this, n3), rout( this, n4),
cin( n5), cout( this, n6)
{

}


BTermio::DataIn::DataIn( BTermio *t, RNode n):
trmio( t), RNodeHandler( n)
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

	if (m->what == EOF)
		node.Shutdown( );
	else {
		m->FindData( "", B_UINT8_TYPE, (const void **)&p, &n);
		if (p) {
			uint i = 0;
			while (i < n) {
				uint c = p[i++];
				if ((c == trmio->cout.t.c_cc[VINTR])
				and (trmio->cout.t.c_lflag & ISIG)) {
					x.clear( );
					trmio->cin.node >> 'sig2';
				}
				else if ((c == trmio->cout.t.c_cc[VQUIT])
				and (trmio->cout.t.c_lflag & ISIG)) {
					x.clear( );
					trmio->cin.node >> 'sig3';
				}
				else if ((c == trmio->cout.t.c_cc[VKILL])
				and (trmio->cout.t.c_lflag & ICANON))
					loop {
						uint c = x.unputb( );
						if (c == EOF)
							break;
						if (c == '\n') {
							x.putb( c);
							break;
						}
						echo( '\b'+0200);
					}
				else if ((c == trmio->cout.t.c_cc[VERASE])
				and (trmio->cout.t.c_lflag & ICANON)) {
					uint c = x.unputb( );
					unless (c == EOF) {
						if (c == '\n') {
							x.putb( c);
							break;
						}
						echo( '\b'+0200);
					}
				}
				else if ((c == trmio->cout.t.c_cc[VEOF])
				and (trmio->cout.t.c_lflag & ICANON))
					x.putb( c);
				else {
					if ((c == '\t')
					and (trmio->cout.t.c_lflag & ICANON))
						echo( c+0200);
					else {
						if ((c == '\r')
						and (trmio->cout.t.c_iflag & ICRNL))
							c = '\n';
						echo( c);
					}
					x.putb( c);
				}
			}
		}
		full = TRUE;
		process( );
		trmio->dout.process( );
	}
}


void
BTermio::DataIn::process( )
{
	uint	n;

	if ((n = x.count( ))
	and (not stopped))
		if (trmio->cout.t.c_lflag & ICANON) {
			uchar *buf = x.getm( n);
			x.ungetm( n);
			for (uint i=0; i<n; ++i)
				if (buf[i] == '\n') {
					send( i+1);
					return;
				}
				else if (buf[i] == trmio->cout.t.c_cc[VEOF]) {
					send( i);
					x.getb( );
					return;
				}
		}
		else
			send( n);
}


void
BTermio::DataIn::send( uint i)
{

	BMessage *m = new BMessage( );
	m->AddData( "", B_UINT8_TYPE, x.getm( i), i);
	node >> m;
	stopped = TRUE;
	if (full) {
// Rico sez to remove this:		trmio->rout.node >> 0;
		full = FALSE;
	}
}


void
BTermio::DataIn::echo( uint c)
{

	if (trmio->cout.t.c_lflag & ECHO)
		trmio->dout.store( c);
}


BTermio::DataOut::DataOut( BTermio *t, RNode n):
trmio( t), RNodeHandler( n)
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
				store( p[i++]);
		}
		full = TRUE;
		process( );
	}
}


void
BTermio::DataOut::store( uint c)
{

	if ((c == '\n')
	and (trmio->cout.t.c_oflag & ONLCR))
		x.putb( '\r');
	x.putb( c);
}


void
BTermio::DataOut::process( )
{

	unless (stopped) {
		if (full) {
			trmio->rin.node >> 0;
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
trmio( t), RNodeHandler( n)
{

	node.Establish( this, "BTermio::ReqIn");
}


void
BTermio::ReqIn::MessageReceived( BMessage *m)
{

	if (m->what == EOF)
		node.Shutdown( );
	else {
		trmio->dout.stopped = FALSE;
		trmio->dout.process( );
	}
}


BTermio::ReqOut::ReqOut( BTermio *t, RNode n):
trmio( t), RNodeHandler( n)
{

	node.Establish( this, "BTermio::ReqOut");
}


void
BTermio::ReqOut::MessageReceived( BMessage *m)
{

	if (m->what == EOF)
		node.Shutdown( );
	else {
		trmio->din.stopped = FALSE;
		trmio->din.process( );
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
trmio( tio), RNodeHandler( n)
{

	memset( &t, 0, sizeof t);
	t.c_iflag = 0;
	t.c_oflag = 0;
	t.c_lflag = 0;
	t.c_cflag = B19200|CS8|CREAD|HUPCL;
	t.c_cc[VERASE] = ctrl( 'H');
	t.c_cc[VKILL] = ctrl( 'U');
	t.c_cc[VEOF] = ctrl( 'D');
	t.c_cc[VINTR] = ctrl( 'C');
	t.c_cc[VQUIT] = ctrl( '\\');
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
	case 'geta':
		m = new BMessage( 'geta');
		m->AddData( "", B_UINT8_TYPE, &t, sizeof t);
		trmio->cin.node >> m;
		break;
	case 'seta':
		const void *p;
		m->FindData( "", B_UINT8_TYPE, &p, &n);
		if (p)
			memcpy( &t, p, sizeof t);
		trmio->cin.node >> 'seta';
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
