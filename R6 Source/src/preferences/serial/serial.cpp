#define NEW_CONFIGURATION_MANAGER 1

#include	<drivers/Drivers.h>
#if NEW_CONFIGURATION_MANAGER
#include	<config_manager.h>
#include	<isapnp.h>
#include	<driver/config_driver.h>
#include	"oldisapnp.h"
#else
#include	<device/isapnp.h>
#include	<driver/pnp_driver.h>
#endif
#include	<Application.h>
#include	<ListView.h>
#include	<ScrollView.h>
#include	<StringView.h>
#include	<Box.h>
#include	<Bitmap.h>
#include	<Alert.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	"rico.h"
#include	"RNode.h"
#include	"Timer.h"
#include	"buttons.h"


struct pnpdev {
	pnpdev			*next;
	isa_device_config	dc;
	char			*name;
	bool			uploaded;
};
struct serial {
	ushort	base,
		intr;
};
struct config {
	config	*next;
	bool	pnp;
	uint	cid,
		did,
		base,
		intr;
	char	*name;
};
struct Serial {
	struct V {
		protected:
		virtual void	winit( BLooper *)	= 0,
				wsend( BMessage *)	= 0,
				whandle( BMessage *)	= 0;
	};
	struct A: BApplication, RNode, virtual V {
		bool	okay;
		protected:
			A( );
		private:
		bool	loadconfig( ),
			showconfig( ),
			getconf( FILE *),
			saveconf( ),
			getconfdefault( ),
			pnpreserve( int),
			getpnp( int),
			upload( ),
			republish( ),
			addpnpdev( uint),
			addjmpdev( BMessage *m),
			rebootcheck( ),
			confeq( config *),
			query( char *, ...),
			loadquery( char *),
			nomem( ),
			toomany( ),
			complain( char *, ...);
		void	ArgvReceived( int32, char **),
			done( bool),
			ReadyToRun( ),
			whandle( BMessage *),
			MessageReceived( BMessage *),
			sendconfig( bool),
			alert( char *, ...),
			usage( ),
			pnpdump( );
		config	*confdup( ),
			*confindex( uint),
			*confdel( config *),
			*confadd( config *, uint = ~0),
			*conf,
			*savedconf;
		pnpdev	*pnp;
		bool	lflag,
			aflag;
	};
	class W: BHandler, RNode, virtual V {
		void	winit( BLooper *),
			wsend( BMessage *),
			MessageReceived( BMessage *);
	};
	struct Object: A, W, virtual V {
			Object( );
	};
};
struct Listing {
	struct V {
		virtual void	pnphandle( BMessage *)		= 0,
				pnpshow( BWindow *, pnpdev *)	= 0,
				jmphandle( BMessage *)		= 0,
				jmpshow( BWindow *)		= 0,
				infoshow( BWindow *)		= 0;
	};
	struct W: BWindow, RNode, virtual V {
				W( RNode);
		void		winit( );
		private:
		void		MessageReceived( BMessage *),
				pnphandle( BMessage *),
				jmphandle( BMessage *);
		BListView	*lv;
		BButton		*mu,
				*md,
				*dd,
				*sc;
	};
	struct PNP: BHandler, RNode, virtual V {
		protected:
			PNP( );
		private:
		void	pnpshow( BWindow *, pnpdev *),
			MessageReceived( BMessage *);
		bool	active;
	};
	struct JMP: BHandler, RNode, virtual V {
		protected:
			JMP( );
		private:
		void	jmpshow( BWindow *),
			MessageReceived( BMessage *);
		bool	active;
	};
	struct I: BHandler, RNode, virtual V {
		protected:
			I( );
		private:
		void	infoshow( BWindow *),
			MessageReceived( BMessage *);
		bool	active;
	};
	struct Object: W, PNP, JMP, I {
			Object( RNode);
	};
};
struct PNPSelector: BWindow, RNode {
			PNPSelector( RNode, pnpdev *, BPoint);
	private:
	void		MessageReceived( BMessage *);
	BListView	*lv;
	BButton		*ba;
};
struct JMPSelector: BWindow, RNode {
			JMPSelector( RNode, BPoint);
	private:
	void		MessageReceived( BMessage *);
	char		*intr,
			*base,
			*desc;
};
struct Info: BWindow, RNode {
		Info( RNode, BPoint);
	void	MessageReceived( BMessage *);
};
struct Boxed {
	struct TV: BTextView {
				TV( char **, BRect, uint);
		void		InsertText( const char *, int32, int32, const text_run_array *),
				DeleteText( int32, int32),
				KeyDown( const char *, int32);
		private:
		const char	**text;
	};
	struct Number: BBox {
		struct Args {
			char	*name,
				*format,
				**text;
			BPoint	org;
			uint	min,
				max,
				def,
				step;
		};
		struct SB: Args, BScrollBar {
				SB( Args, Number *);
			void	AttachedToWindow( ),
				ValueChanged( float);
			private:
			Number	*number;
		};
				Number( Args);
		void		show( char *, uint);
		private:
		BTextView	*t;
	};
	struct String: BBox {
		struct Args {
			char	*name,
				**text;
			BPoint	org;
		};
				String( Args);
		void		show( char *, uint);
		private:
		BTextView	*t;
	};
};
struct Alert: BAlert, RNode {
		Alert( char *, char *, alert_type, bool);
	private:
	void	Show( ),
		MessageReceived( BMessage *);
	bool	timeout;
};
struct LQuery: BAlert, RNode {
		LQuery( char *, bool *);
	private:
	void	Show( ),
		MessageReceived( BMessage *);
};


static const char	CONF[]		= "/etc/serialconf";
static const char	SER[]		= "/dev/config/serial";
#if NEW_CONFIGURATION_MANAGER
static const char	PNP[]		= "/dev/" CM_DEVICE_NAME;
#else
static const char	PNP[]		= "/dev/misc/pnp";
#endif

static const rgb_color GRAY = {
	218, 218, 218
};


main( )
{
	Serial::Object	a;

	return (a.okay? 0: 1);
}


Serial::Object::Object( )
{

	Run( );
}


Serial::A::A( ):
BApplication( "application/x-vnd.Be-Serial")
{

	okay = TRUE;
	aflag = FALSE;
	lflag = FALSE;
	conf = 0;
	pnp = 0;
	savedconf = 0;
}


void
Serial::A::ArgvReceived( int32, char **av)
{

	++av;
	while (char *p = *av++) {
		unless (*p++ == '-')
			usage( );
		while (*p)
			switch (*p++) {
			case 'l':
				lflag = TRUE;
				break;
			case 'a':
				aflag = TRUE;
				break;
			default:
				usage( );
				return;
			}
	}
}


void
Serial::A::ReadyToRun( )
{

	SetDestination( *this);
	Establish( 0, this, "Serial::A");
	if (lflag) {
		if ((aflag)
		or (loadquery( "Initializing Serial Devices...")))
			okay = loadconfig( );
		Quit( );
	}
	else {
		winit( this);
		unless (showconfig( ))
			Quit( );
	}
}


void
Serial::A::MessageReceived( BMessage *m)
{

	//printf( "Serial::A::MessageReceived: what=%d (%.4s)\n", m->what, &m->what);
}


void
Serial::W::winit( BLooper *l)
{
	RNode	n;

	n.SetDestination( *this);
	SetDestination( n);
	new Listing::Object( n);
	Establish( this, l, "W");
}


void
Serial::W::wsend( BMessage *m)
{

	*this >> m;
}


void
Serial::W::MessageReceived( BMessage *m)
{

	//printf( "Serial::W::MessageReceived: what=%d (%.4s)\n", m->what, &m->what);
	whandle( m);
}


void
Serial::A::whandle( BMessage *m)
{
	int32	i;
	int	fd;

	switch (m->what) {
	case 'mu':
		if ((m->FindInt32( "", &i) == B_OK)
		and (i)) {
			confadd( confdel( confindex( i)), i-1);
			sendconfig( FALSE);
		}
		break;
	case 'md':
		if (m->FindInt32( "", &i) == B_OK) {
			confadd( confdel( confindex( i)), i+1);
			sendconfig( FALSE);
		}
		break;
	case 'del':
		if (m->FindInt32( "", &i) == B_OK) {
			confdel( confindex( i));
			sendconfig( FALSE);
		}
		break;
	case 'pnp':
		fd = open( PNP, O_RDONLY);
		if (fd < 0)
			complain( "Cannot open %s (%s).", PNP, strerror( errno));
		else {
			if (getpnp( fd)) {
				m = new BMessage( 'PnP');
				m->AddPointer( "", pnp);
				wsend( m);
				//pnpdump( );
			}
			close( fd);
		}
		break;
	case '+pnp':
		if (m->FindInt32( "", &i) == B_OK)
			addpnpdev( i);
		break;
	case '+jmp':
		addjmpdev( m);
		break;
	case 'def':
		getconfdefault( );
		sendconfig( FALSE);
		break;
	case 'save':
		saveconf( );
		break;
	case EOF:
		if ((not confeq( savedconf))
		and (query( "Your configuration will now be saved.")))
			saveconf( );
		Quit( );
	}
}


bool
Serial::A::addpnpdev( uint i)
{
	pnpdev	*p;

	for (p=pnp; i; --i)
		p = p->next;
	config *c = new config;
	c->pnp = TRUE;
	c->cid = p->dc.card_id.id;
	c->did = p->dc.logical_device_id.id;
	c->name = strdup( p->name);

	int n = 0;
	for (p=pnp; p; p=p->next)
		if ((p->dc.card_id.id == c->cid)
		and (p->dc.logical_device_id.id == c->did)
		and (streq( p->name, c->name)))
			++n;
	for (config *a=conf; a; a=a->next)
		if ((a->pnp)
		and (a->cid == c->cid)
		and (a->did == c->did)
		and (streq( a->name, c->name)))
			--n;
	if (n <= 0)
		return (complain( "This card is already configured."));

	confadd( c);
	sendconfig( TRUE);
	return (TRUE);
}


bool
Serial::A::addjmpdev( BMessage *m)
{
	static struct {
		uint	base,
			end;
		char	*label;
	} iop_used[] = {
		0x000, 0x020, "1st DMA controller",
		0x020, 0x040, "1st interrupt controller",
		0x040, 0x060, "system timer",
		0x060, 0x070, "keyboard",
		0x070, 0x072, "realtime clock",
		0x080, 0x0A0, "DMA page registers",
		0x0A0, 0x0C0, "2nd interrupt controller",
		0x0C0, 0x0E0, "2nd DMA controller",
		0x0F0, 0x100, "math coprocessor",
		0x170, 0x178, "2nd IDE controller",
		0x1F0, 0x1F8, "1st IDE controller",
		0x376, 0x377, "2nd IDE controller",
		0x378, 0x380, "2nd parallel port",
		0x3BC, 0x3C0, "1st parallel port",
		0x3C0, 0x3E0, "VGA display registers",
		0x3F0, 0x3F6, "floppy",
		0x3F6, 0x3F7, "1st IDE controller",
		0x3F7, 0x3F8, "floppy",
	};
	static char *irq_used[] = {
		"the system timer",
		"the keyboard",
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		"a PS/2 mouse (if you have one)",
		"the math library",
		"the IDE drive",
		"the IDE drive"
	};
	config	*a;

	unless (a = new config)
		return (nomem( ));
	a->pnp = FALSE;
	uint u;
	char _;
	char *s;
	m->FindString( "intr", &s);
	unless (sscanf( s, "%u %c", &u, &_) == 1)
		return (complain( "%s is not a valid device IRQ.", s));
	a->intr = u;
	m->FindString( "base", &s);
	unless (sscanf( s, "0x%x %c", &u, &_) == 1)
		return (complain( "%s is not a valid I/O port.", s));
	a->base = u;
	m->FindString( "desc", &s);
	a->name = strdup( s);
	if (a->intr == 2)
		return (complain( "Interrupt 2 is not available."));
	unless (a->intr < nel( irq_used))
		return (complain( "Interrupts must lie between 0 and 15."));
	unless (a->base < 0xFFF8)
		return (complain( "I/O addresses must lie between 0 and 0xFFF8."));
	for (config *c=conf; c; c=c->next)
		unless (c->pnp)
			if (abs( c->base-a->base) < 8)
				return (complain( "Two jumpered serial devices have conflicting I/O ports."));
	for (config *c=conf; c; c=c->next)
		unless (c->pnp)
			if (c->intr == a->intr) {
				unless (query( "Two jumpered serial devices require the same IRQ.  Except amongst PCI cards and multi-port serial cards, this is usually a mistake."))
					return (FALSE);
				break;
			}
	if (irq_used[a->intr]) {
		unless (query( "%u is usually an interrupt for %s.", a->intr, irq_used[a->intr]))
			return (FALSE);
	}
	for (uint i=0; i<nel( iop_used); ++i)
		unless ((iop_used[i].end <= a->base)
		or (a->base+8 <= iop_used[i].base)) {
			unless (query( "I/O port 0x%03X is normally reserved for the %s.", a->base, iop_used[i].label))
				return (FALSE);
			break;
		}
	confadd( a);
	sendconfig( TRUE);
	return (TRUE);
}


bool
Serial::A::showconfig( )
{

	aflag = FALSE;
	if (FILE *f = fopen( CONF, "r")) {
		if (getconf( f))
			savedconf = confdup( );
		else unless (getconfdefault( ))
			return (FALSE);
		fclose( f);
	}
	else {
		unless (getconfdefault( ))
			return (FALSE);
		savedconf = confdup( );
	}
	sendconfig( FALSE);
	return (TRUE);
}


void
Serial::A::sendconfig( bool added)
{

	BMessage *m = new BMessage( 'conf');
	m->AddPointer( "", confdup( ));
	if (added)
		m->AddBool( "add", TRUE);
	unless (confeq( savedconf))
		m->AddBool( "savable", TRUE);
	wsend( m);
}


bool
Serial::A::loadconfig( )
{

	int pnpfd = open( PNP, O_RDONLY);
	if (pnpfd < 0)
		return (complain( "Cannot open %s (%s).", PNP, strerror( errno)));
	if (FILE *f = fopen( CONF, "r")) {
		unless ((getconf( f))
		and (pnpreserve( pnpfd))
		and (getpnp( pnpfd))
		and (upload( )))
			return (FALSE);
		fclose( f);
	}
	//pnpdump( );
	return (TRUE);
}


bool
Serial::A::getconf( FILE *f)
{
	char	line[BUFSIZ],
		buff[BUFSIZ];
	uint	i1,
		i2;

	uint l = 0;
	while (fgets( line, sizeof line, f)) {
		++l;
		config *c = new config;
		unless (c)
			return (nomem( ));
		if (sscanf( line, "PNP:0x%x:0x%x:%[^\n]", &i1, &i2, buff) == 3) {
			c->pnp = TRUE;
			c->cid = i1;
			c->did = i2;
		}
		else if (sscanf( line, "JMP:0x%x:%u:%[^\n]", &i1, &i2, buff) == 3) {
			c->pnp = FALSE;
			c->base = i1;
			c->intr = i2;
		}
		else
			return (complain( "Syntax error in %s, line %d.", CONF, l));
		unless (c->name = strdup( buff))
			return (nomem( ));
		confadd( c);
	}
	fclose( f);
	return (TRUE);
}


bool
Serial::A::saveconf( )
{
	FILE	*f;

	unless (f = fopen( CONF, "w"))
		return (complain( "Cannot write %s (%s).", CONF, strerror( errno)));
	for (config *c=conf; c; c=c->next)
		if (c->pnp)
			fprintf( f, "PNP:0x%X:0x%X:%s\n", c->cid, c->did, c->name);
		else
			fprintf( f, "JMP:0x%X:%u:%s\n", c->base, c->intr, c->name);
	fclose( f);
	if (rebootcheck( ))
		alert( "Reboot machine before using serial devices.");
	else if (upload( ))
		alert( "PnP hardware is ready for use (no need to reboot).");
	savedconf = confdup( );
	return (TRUE);
}


bool
Serial::A::getconfdefault( )
{
	struct {
		ushort	base,
			intr;
		char	*name;
	} dtab[] = {
		0x3F8, 4, "1st built-in",
		0x2F8, 3, "2nd built-in",
	};

	while (config *c = conf)
		free( confdel( c));
	for (uint i=0; i<nel( dtab); ++i) {
		config *c = new config;
		unless (c)
			return (nomem( ));
		c->pnp = FALSE;
		c->base = dtab[i].base;
		c->intr = dtab[i].intr;
		unless (c->name = strdup( dtab[i].name))
			return (nomem( ));
		confadd( c);
	}
	return (TRUE);
}


bool
Serial::A::pnpreserve( int fd)
{
#if NEW_CONFIGURATION_MANAGER
	return (TRUE);
#else
	pnpargs	a;
	config	*c;

	for (c=conf; c; c=c->next)
		unless (c->pnp) {
			a.irqd.irq = c->intr;
			a.irqd.irq_type = HTES;
			if (ioctl( fd, PNP_RESERVE_IRQ, &a) < 0)
				return (complain( "Failed to reserve an IRQ from the PnP system (%s).", strerror( errno)));
			a.iod.base = c->base;
			a.iod.len = 8;
			if (ioctl( fd, PNP_RESERVE_IO_PORT_RANGE, &a) < 0)
				return (complain( "Failed to reserve I/O port from the PnP system (%s).", strerror( errno)));
		}
	if (ioctl( fd, PNP_CONFIGURE_ALL_DEVICES) < 0)
		return (complain( "Failed to configure PnP devices (%s).", strerror( errno)));
	return (TRUE);
#endif
}

#if NEW_CONFIGURATION_MANAGER

static
status_t count_resource_descriptors_of_type(
    const struct device_configuration *c, resource_type type)
{
    uint32 i, count = 0;

    if (!c) return EINVAL;

    for (i=0;i<c->num_resources;i++)
        if (c->resources[i].type == type)
            count++;
    return count;
}

static
status_t get_nth_resource_descriptor_of_type(
    const struct device_configuration *c, uint32 n, resource_type type,
    resource_descriptor *d, uint32 size)
{
    uint32 i;
    if (!c || !d) return EINVAL;

    for (i=0;i<c->num_resources;i++)
        if ((c->resources[i].type == type) && (n-- == 0)) {
            if (size > sizeof(resource_descriptor))
                size = sizeof(resource_descriptor);
            memcpy(d, c->resources + i, size);
            return B_OK;
        }

    return ENOENT;
}

static
uchar mask_to_value(uint32 mask)
{
  uchar value;
  if (!mask) return 0;
  for (value=0,mask>>=1;mask;value++,mask>>=1)
    ;
  return value;
}

static status_t
convert_isa_info(isa_device_config *dev, struct isa_device_info *info,
		struct device_configuration *config)
{
  int32 config_size, i, j;
  resource_descriptor r;

  dev->card_id.id = info->card_id;
  dev->logical_device_id.id = info->logical_device_id;
  dev->num_compatible_device_ids = info->num_compatible_ids;
  for (i=0;(i<info->num_compatible_ids) && (i<B_MAX_COMPATIBLE_IDS) &&
       (i<MAX_COMPATIBLE_DEVICE_ID);i++)
    dev->compatible_device_ids[i].id = info->compatible_ids[i];
  strncpy(dev->card_name, dev->card_name, MAX_ISA_PNP_NAME_LEN-1);
  strncpy(dev->logical_device_name, dev->logical_device_name,
          MAX_ISA_PNP_NAME_LEN-1);

  j = count_resource_descriptors_of_type(config, B_IRQ_RESOURCE);
  if (j > MAX_ISA_PNP_IRQS) j = MAX_ISA_PNP_IRQS;
  for (i=0;i<j;i++) {
    get_nth_resource_descriptor_of_type(config, i, B_IRQ_RESOURCE, &r,
      sizeof(resource_descriptor));
    dev->irqs[i].irq = mask_to_value(r.d.m.mask);
    dev->irqs[i].irq_type = (IRQ_TYPE)r.d.m.cookie;
  }
  dev->num_irqs = j;

  j = count_resource_descriptors_of_type(config, B_DMA_RESOURCE);
  if (j > MAX_ISA_PNP_IRQS) j = MAX_ISA_PNP_DMAS;
  for (i=0;i<j;i++) {
    get_nth_resource_descriptor_of_type(config, i, B_DMA_RESOURCE, &r,
      sizeof(resource_descriptor));
    dev->dma_channels[i].channel = mask_to_value(r.d.m.mask);
    dev->dma_channels[i].flags = r.d.m.cookie;
  }
  dev->num_dma_channels = j;

  j = count_resource_descriptors_of_type(config, B_IO_PORT_RESOURCE);
  if (j > MAX_ISA_PNP_IRQS) j = MAX_ISA_PNP_IO_PORT_RANGES;
  for (i=0;i<j;i++) {
    get_nth_resource_descriptor_of_type(config, i, B_IO_PORT_RESOURCE, &r,
      sizeof(resource_descriptor));
    dev->io_port_ranges[i].base = r.d.r.minbase;
    dev->io_port_ranges[i].len = r.d.r.len;
  }
  dev->num_io_port_ranges = j;

  j = count_resource_descriptors_of_type(config, B_MEMORY_RESOURCE);
  if (j > MAX_ISA_PNP_IRQS) j = MAX_ISA_PNP_MEMORY_RANGES;
  for (i=0;i<j;i++) {
    get_nth_resource_descriptor_of_type(config, i, B_MEMORY_RESOURCE, &r,
      sizeof(resource_descriptor));
    dev->memory_ranges[i].base = r.d.r.minbase;
    dev->memory_ranges[i].len = r.d.r.len;
    dev->memory_ranges[i].flags = r.d.r.cookie;
  }
  dev->num_memory_ranges = j;

  free(config);

  return B_OK;
}

#endif

bool
Serial::A::getpnp( int fd)
{
	pnpdev	*p,
		*ep;
	char	name[BUFSIZ];

	unless (pnp) {
#if NEW_CONFIGURATION_MANAGER
		uint64 cookie = 0;
		struct isa_device_info info;
		while (1) {
			struct cm_ioctl_data d;
			int32 config_size;
			struct device_configuration *config;

			d.magic = CM_GET_NEXT_DEVICE_INFO;
			d.bus = B_ISA_BUS;
			d.cookie = cookie;
			d.info = &info;
			d.info_len = sizeof(struct isa_device_info);
			if (ioctl(fd, d.magic, &d, sizeof(d)) != B_OK)
				break;

			cookie = d.cookie;
			d.magic = CM_GET_SIZE_OF_CURRENT_CONFIGURATION_FOR;
			if ((config_size = ioctl(fd, d.magic, &d, sizeof(d))) < B_OK)
				break;

			config = (struct device_configuration *)malloc(config_size);
			if (!config)
				return (nomem( ));

			d.magic = CM_GET_CURRENT_CONFIGURATION_FOR;
			d.info = config;
			d.info_len = config_size;
			if (ioctl(fd, d.magic, &d, sizeof(d)) < B_OK) {
				free(config);
				break;
			}

			sprintf( name, "%s / %s", info.card_name, info.logical_device_name);
			unless ((p = new pnpdev)
			and (p->name = strdup( name))) {
				free(config);
				return (nomem( ));
			}
			convert_isa_info(&(p->dc), &info, config);
			p->next = 0;
#else
		uint n = 0;
		loop {
			pnpargs a;
			a.i = n++;
			unless (ioctl( fd, PNP_GET_NTH_ISA_DEVICE_CONFIG, &a, sizeof a) == B_OK)
				break;
			sprintf( name, "%s / %s", a.dc.card_name, a.dc.logical_device_name);
			unless ((p = new pnpdev)
			and (p->name = strdup( name)))
				return (nomem( ));
			p->dc = a.dc;
#endif
			p->next = 0;
			if (pnp)
				ep->next = p;
			else
				pnp = p;
			ep = p;
		}
	}
	return (TRUE);
}


bool
Serial::A::rebootcheck( )
{

	config *c = conf;
	config *s = savedconf;
	while (c) {
		if (s) {
			unless ((s->pnp == c->pnp)
			and (streq( s->name, c->name)))
				return (TRUE);
			if (s->pnp) {
				unless ((s->cid == c->cid)
				and (s->did == c->did))
					return (TRUE);
			}
			else {
				unless ((s->base == c->base)
				and (s->intr == c->intr))
					return (TRUE);
			}
		}
		else {
			while (c) {
				unless (c->pnp)
					return (TRUE);
				c = c->next;
			}
			return (FALSE);
		}
		c = c->next;
		s = s->next;
	}
	return (TRUE);
}


bool
Serial::A::upload( )
{
	serial	stab[50];
	config	*c;

	for (pnpdev *p=pnp; p; p=p->next)
		p->uploaded = FALSE;
	uint n = 0;
	memset( stab, 0, sizeof stab);
	for (c=conf; c; c=c->next) {
		if (c->pnp) {
			pnpdev *p = pnp;
			loop {
				unless (p) {
					alert( "Could not find a PnP card (%s).", c->name);
					stab[n].base = 0xABBA;
					break;
				}
				unless (p->uploaded)
					if ((c->cid == p->dc.card_id.id)
					and (c->did == p->dc.logical_device_id.id)
					and (streq( c->name, p->name))) {
						unless (n < nel( stab))
							return (toomany( ));
						stab[n].base = p->dc.io_port_ranges[0].base;
						stab[n].intr = p->dc.irqs[0].irq;
						p->uploaded = TRUE;
						break;
					}
				p = p->next;
			}
		}
		else {
			unless (n < nel( stab))
				return (toomany( ));
			stab[n].base = c->base;
			stab[n].intr = c->intr;
		}
		++n;
	}

	/*
	 * at least one device is required
	 */
	unless (n)
		stab[n++].base = 0xABBA;

	int fd = open( SER, O_RDWR);
	if (fd < 0)
		return (complain( "Cannot open %s (%s).", SER, strerror( errno)));
	n *= sizeof *stab;
	if (write( fd, stab, sizeof stab) < n)
		return (toomany( ));
	close( fd);
	return (republish( ));
}


bool
Serial::A::republish( )
{

	int fd = open( "/dev", O_WRONLY);
	if (fd < 0)
		return (complain( "Cannot open /dev (%s).", strerror( errno)));
	write( fd, "zz", 2);
	close( fd);
	return (TRUE);
}


config	*
Serial::A::confdup( )
{
	config	*copy,
		*c;

	copy = conf;
	conf = 0;
	for (config *p=copy; p; p=p->next) {
		unless (c = new config)
			return ((config *) nomem( ));
		*c = *p;
		unless (c->name = strdup( c->name))
			return ((config *) nomem( ));
		confadd( c);
	}
	return (copy);
}


bool
Serial::A::confeq( config *a)
{

	for (config *c=conf; c; c=c->next) {
		unless ((a)
		and (a->pnp == c->pnp)
		and (streq( a->name, c->name)))
			return (FALSE);
		if (a->pnp) {
			unless ((a->cid == c->cid)
			and (a->did == c->did))
				return (FALSE);
		}
		else {
			unless ((a->base == c->base)
			and (a->intr == c->intr))
				return (FALSE);
		}
		a = a->next;
	}
	return (a == 0);
}


config	*
Serial::A::confindex( uint i)
{
	config	*p;

	for (p=conf; p && i; p=p->next, --i)
		;
	return (p);
}


config	*
Serial::A::confdel( config *c)
{

	config *q = conf;
	if (q == c)
		conf = q->next;
	else
		loop {
			config *p = q;
			q = q->next;
			if (q == c) {
				p->next = q->next;
				break;
			}
		}
	return (c);
}


config	*
Serial::A::confadd( config *c, uint i)
{

	config *q = conf;
	unless ((q)
	and (i))
		conf = c;
	else
		loop {
			config *p = q;
			unless ((q = q->next)
			and (--i)) {
				p->next = c;
				break;
			}
		}
	c->next = q;
	return (c);
}


bool
Serial::A::query( char *mesg, ...)
{
	va_list	ap;
	char	s[BUFSIZ];

	va_start( ap, mesg);
	vsprintf( s, mesg, ap);
	BAlert *a = new BAlert( "Serial Query", s, "Cancel", "Proceed", 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	a->SetShortcut( 0, B_ESCAPE);
	return (a->Go( ));
}


bool
Serial::A::loadquery( char *mesg)
{
	bool	r;

	new LQuery( mesg, &r);
	return (r);
}


void
Serial::A::alert( char *mesg, ...)
{
	va_list	ap;
	char	s[BUFSIZ];

	if (aflag) {
		fprintf( stderr, "Serial (warning): ");
		va_start( ap, mesg);
		vfprintf( stderr, mesg, ap);
		fprintf( stderr, "\n");
	}
	else {
		va_start( ap, mesg);
		vsprintf( s, mesg, ap);
		new Alert( s, "Continue", B_WARNING_ALERT, lflag);
	}
}


void
Serial::A::usage( )
{
	char	s[BUFSIZ];

	sprintf( s, "usage: Serial [ -l ] [ -a ]");
	if (aflag)
		fprintf( stderr, "%s\n", s);
	else
		new Alert( s, "Exit", B_STOP_ALERT, lflag);
	Quit( );
}


bool
Serial::A::toomany( )
{

	return (complain( "Too many serial devices to configure."));
}


bool
Serial::A::nomem( )
{

	return (complain( "Out of memory."));
}


bool
Serial::A::complain( char *mesg, ...)
{
	va_list	ap;
	char	s[BUFSIZ];

	if (aflag) {
		fprintf( stderr, "Serial: ");
		va_start( ap, mesg);
		vfprintf( stderr, mesg, ap);
		fprintf( stderr, "\n");
	}
	else {
		va_start( ap, mesg);
		vsprintf( s, mesg, ap);
		new Alert( s, lflag? "Exit": "Sorry", B_STOP_ALERT, lflag);
	}
	return (FALSE);
}


Listing::Object::Object( RNode n):
W( n)
{

	winit( );
}


Listing::W::W( RNode n):
RNode( n), BWindow( BRect( 0, 0, 560, 270), "Serial Configuration", B_TITLED_WINDOW, B_NOT_ZOOMABLE|B_NOT_RESIZABLE)
{

}


void
Listing::W::winit( )
{

	MoveTo( 70, 70);

	BView *v = new BView( Bounds( ), 0, 0, B_WILL_DRAW);
	v->SetViewColor( GRAY);

	BRect r = Bounds( );
	r.right -= B_V_SCROLL_BAR_WIDTH + 60;
	r.InsetBy( 20, 20);
	r.top += 15;
	r.bottom -= 65;
	lv = new BListView( r, 0);
	lv->SetFont( be_fixed_font);
	lv->SetSelectionMessage( new BMessage( 'sel'));

	BScrollView *sv = new BScrollView( 0, lv, 0, 0, FALSE, TRUE);
	v->AddChild( sv);

	r = BRect( 90+20, 200, 90+170, 220);
	BButton *ap = new BButton( r, 0, "Add Plug-and-Play Device...", new BMessage( 'pnp'));
	v->AddChild( ap);

	r.OffsetBy( 0, 35);
	BButton *aj = new BButton( r, 0, "Add Jumpered Device...", new BMessage( 'jmp'));
	v->AddChild( aj);

	r.OffsetBy( 160, -35);
	v->AddChild( new BButton( r, 0, "Use Default Configuration", new BMessage( 'def')));

	r.OffsetBy( 0, 35);
	dd = new BButton( r, 0, "Delete Device", new BMessage( 'del'));
	dd->SetEnabled( FALSE);
	v->AddChild( dd);

	mu = new ArrowButton( BPoint( 495, 73), FALSE, 'mu');
	mu->SetEnabled( FALSE);
	v->AddChild( mu);

	md = new ArrowButton( BPoint( 495, 115), TRUE, 'md');
	md->SetEnabled( FALSE);
	v->AddChild( md);

	r = BRect( 25, 10, 400, 30);
	BView *s = new BStringView( r, 0, "Device        I/O Port    IRQ    Description");
	s->SetFontSize( 11);
	v->AddChild( s);

	r = BRect( 460, 225, 520, 255);
	sc = new BButton( r, 0, "Save", new BMessage( 'save'));
	sc->MakeDefault( TRUE);
	sc->SetEnabled( FALSE);
	v->AddChild( sc);

	v->AddChild( new InfoButton( BPoint( 30, 200), 'info'));

	AddChild( v);
	Show( );
	Establish( 0, this, "Listing");
}


void
Listing::W::MessageReceived( BMessage *m)
{
	void	*v;
	uint	i;
	int32	sel;
	char	s0[40],
		s1[BUFSIZ];
	pnpdev	*pnp;

	//printf( "Listing::W::MessageReceived: what=%d (%.4s)\n", m->what, &m->what);
	sel = lv->CurrentSelection( );
	switch (m->what) {
	case 'conf':
		if ((m->FindPointer( "", &v) == B_OK)) {
			i = 0;
			for (config *c=(config *)v; c; c=c->next) {
				sprintf( s0, "serial%d", i+1);
				if (c->pnp)
					sprintf( s1, "%-8s ---PnP---  %s", s0, c->name);
				else
					sprintf( s1, "%-8s 0x%03X  %2d  %s", s0, c->base, c->intr, c->name);
				BListItem *li;
				if (li = lv->ItemAt( i)) {
					unless (streq( s1, ((BStringItem *)li)->Text( ))) {
						lv->AddItem( new BStringItem( s1), i);
						lv->RemoveItem( i+1);
					}
				}
				else
					lv->AddItem( new BStringItem( s1), i);
				++i;
			}
			while (i < lv->CountItems( ))
				lv->RemoveItem( i);
			if (m->HasBool( "add"))
				lv->Select( lv->CountItems( )-1);
			else unless (sel < 0)
				lv->Select( sel);
			lv->ScrollToSelection( );
			sc->SetEnabled( m->HasBool( "savable"));
		}
		break;
	case 'sel':
		dd->SetEnabled( sel>=0);
		mu->SetEnabled( sel>=0);
		md->SetEnabled( sel>=0);
		break;
	case 'mu':
		if (sel) {
			lv->Select( sel-1);
			BMessage *m = new BMessage( 'mu');
			m->AddInt32( "", sel);
			*this >> m;
		}
		break;
	case 'md':
		if (sel+1 < lv->CountItems( )) {
			lv->Select( sel+1);
			m = new BMessage( 'md');
			m->AddInt32( "", sel);
			*this >> m;
		}
		break;
	case 'del':
		lv->RemoveItem( sel);
		m = new BMessage( 'del');
		m->AddInt32( "", sel);
		*this >> m;
		break;
	case 'pnp':
		*this >> 'pnp';
		break;
	case 'PnP':
		if (m->FindPointer( "", (void **)&pnp) == B_OK)
			pnpshow( this, pnp);
		break;
	case 'jmp':
		jmpshow( this);
		break;
	case 'def':
		*this >> 'def';
		break;
	case 'info':
		infoshow( this);
		break;
	case 'save':
		*this >> 'save';
		Quit( );
	default:
		BWindow::MessageReceived( m);
	}
}


void
Listing::W::pnphandle( BMessage *m)
{

	//printf( "Listing::W::pnphandle: what=%d (%.4s)\n", m->what, &m->what);
	switch (m->what) {
	case 'add':
		m = DetachCurrentMessage( );
		m->what = '+pnp';
		*this >> m;
	}
}


void
Listing::W::jmphandle( BMessage *m)
{

	//printf( "Listing::W::jmphandle: what=%d (%.4s)\n", m->what, &m->what);
	switch (m->what) {
	case 'add':
		m = DetachCurrentMessage( );
		m->what = '+jmp';
		*this >> m;
	}
}


Listing::PNP::PNP( )
{

	active = FALSE;
}


void
Listing::PNP::pnpshow( BWindow *w, pnpdev *pnp)
{

	if (active)
		*this >> 'acti';
	else {
		RNode	n;
		*(RNode *)this = RNode( );
		n.SetDestination( *this);
		SetDestination( n);
		new PNPSelector( n, pnp, w->Frame( ).LeftTop( )+BPoint( 50, 50));
		active = TRUE;
		Establish( this, w, "Listing::PNP");
	}
}


void
Listing::PNP::MessageReceived( BMessage *m)
{

	switch (m->what) {
	case EOF:
		active = FALSE;
		break;
	default:
		pnphandle( m);
	}
}


Listing::JMP::JMP( )
{

	active = FALSE;
}


void
Listing::JMP::jmpshow( BWindow *w)
{

	if (active)
		*this >> 'acti';
	else {
		RNode	n;
		*(RNode *)this = RNode( );
		n.SetDestination( *this);
		SetDestination( n);
		new JMPSelector( n, w->Frame( ).LeftTop( )+BPoint( 50, 50));
		active = TRUE;
		Establish( this, w, "Listing::JMP");
	}
}


void
Listing::JMP::MessageReceived( BMessage *m)
{

	switch (m->what) {
	case EOF:
		active = FALSE;
		break;
	default:
		jmphandle( m);
	}
}


Listing::I::I( )
{

	active = FALSE;
}


void
Listing::I::infoshow( BWindow *w)
{

	if (active)
		*this >> 'acti';
	else {
		RNode	n;
		*(RNode *)this = RNode( );
		n.SetDestination( *this);
		SetDestination( n);
		new Info( n, w->Frame( ).LeftTop( )+BPoint( 50, 50));
		active = TRUE;
		Establish( this, w, "Listing::I");
	}
}


void
Listing::I::MessageReceived( BMessage *m)
{

	if (m->what == EOF)
		active = FALSE;
}


PNPSelector::PNPSelector( RNode n, pnpdev *p, BPoint org):
RNode( n), BWindow( BRect( 0, 0, 500, 195), "Available Plug-and-Play Serial Devices", B_TITLED_WINDOW, B_NOT_ZOOMABLE|B_NOT_RESIZABLE)
{
	char	a[BUFSIZ];

	MoveTo( org);

	BRect r = Bounds( );
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.InsetBy( 20, 20);
	r.top += 15;
	r.bottom -= 45;
	lv = new BListView( r, 0);
	lv->SetFont( be_fixed_font);
	lv->SetSelectionMessage( new BMessage( 'sel'));
	lv->SetInvocationMessage( new BMessage( 'add'));
	while (p) {
		sprintf( a, " 0x%03X  %2d  %s", p->dc.io_port_ranges[0].base, p->dc.irqs[0].irq, p->name);
		BStringItem *si = new BStringItem( a);
		if ((p->dc.num_irqs == 1)
		and (p->dc.num_dma_channels == 0)
		and (p->dc.num_io_port_ranges == 1)
		and (p->dc.num_memory_ranges == 0)
		and (p->dc.io_port_ranges[0].len == 8))
			si->SetEnabled( TRUE);
		else
			si->SetEnabled( FALSE);
		lv->AddItem( si);
		p = p->next;
	}

	BScrollView *sv = new BScrollView( 0, lv, 0, 0, FALSE, TRUE);

	r = BRect( 225, 150, 285, 180);
	ba = new BButton( r, 0, "Add", new BMessage( 'add'));
	ba->SetEnabled( FALSE);
	ba->MakeDefault( TRUE);

	r = BRect( 20, 250, 400, 350);

	BView *v = new BView( Bounds( ), 0, 0, B_WILL_DRAW);
	v->SetViewColor( GRAY);
	v->AddChild( sv);
	v->AddChild( ba);
	AddChild( v);

	BView *s = new BStringView( BRect( 25, 10, 400, 30), 0, "I/O Port    IRQ    Description");
	s->SetFontSize( 11);
	v->AddChild( s);

	Show( );
	Establish( 0, this, "PNPSelector");
}


void
PNPSelector::MessageReceived( BMessage *m)
{
	int	i;

	switch (m->what) {
	case 'acti':
		Activate( );
		break;
	case 'sel':
		ba->SetEnabled( lv->CurrentSelection( ) >= 0);
		break;
	case 'add':
		i = lv->CurrentSelection( );
		unless (i < 0) {
			m = new BMessage( 'add');
			m->AddInt32( "", i);
			*this >> m;
			Quit( );
		}
		break;
	default:
		BWindow::MessageReceived( m);
	}
}




JMPSelector::JMPSelector( RNode node, BPoint org):
RNode( node), BWindow( BRect( 0, 0, 440, 270), "New Jumpered Serial Device", B_TITLED_WINDOW, B_NOT_ZOOMABLE|B_NOT_RESIZABLE)
{
	Boxed::Number::Args	n;
	Boxed::String::Args	s;

	MoveTo( org);
	BRect r = Bounds( );

	BView *v = new BView( Bounds( ), 0, 0, B_WILL_DRAW);
	v->SetViewColor( GRAY);

	n.name = "IRQ";
	n.org = BPoint( 20, 10);
	n.format = " %d";
	n.min = 0;
	n.max = 15;
	n.def = 5;
	n.step = 10;
	n.text = &intr;
	v->AddChild( new Boxed::Number( n));

	n.name = "I/O Ports";
	n.org = BPoint( 20, 80);
	n.format = "0x%03X";
	n.min = 0x100;
	n.max = 0x400;
	n.def = 0x03E8;
	n.step = 0x008;
	n.text = &base;
	v->AddChild( new Boxed::Number( n));

	s.name = "Helpful Description";
	s.org = BPoint( 20, 150);
	s.text = &desc;
	v->AddChild( new Boxed::String( s));

	r = BRect( 225, 150, 285, 180);
	r.OffsetBy( -40, 75);
	BButton *b = new BButton( r, 0, "Add", new BMessage( 'add'));
	b->MakeDefault( TRUE);
	v->AddChild( b);

	AddChild( v);
	Show( );
	Establish( 0, this, "JMPSelector");
}


void
JMPSelector::MessageReceived( BMessage *m)
{

	//printf( "JMPSelector::MessageReceived: what=%d (%.4s)\n", m->what, &m->what);
	switch (m->what) {
	case 'acti':
		Activate( );
		break;
	case 'irq':
		m->PrintToStream( );
		//sscanf( );
		break;
	case 'add':
		m = new BMessage( 'add');
		m->AddString( "intr", intr);
		m->AddString( "base", base);
		if (desc[0])
			m->AddString( "desc", desc);
		else
			m->AddString( "desc", "-");
		*this >> m;
		Quit( );
		break;
	default:
		BWindow::MessageReceived( m);
	}
}


Info::Info( RNode n, BPoint org):
RNode( n), BWindow( BRect( 0, 0, 640, 375), "Info About Configuring Serial Devices", B_TITLED_WINDOW, B_NOT_ZOOMABLE|B_NOT_RESIZABLE)
{
	static char *info[] = {
		"+Platforms",
		"",
		"At this time, the Z configuration tool is for use with PC hardware only.",
		"",
		"+Plug-and-Play Cards",
		"",
		"To operate a PnP serial card or modem, first power down and install it",
		"physically.  Then use Z to identify the card.  The card will appear in",
		"the PnP list only if conflicts with other cards (both PnP and jumpered)",
		"can be resolved.  If the card is later removed, or if it cannot be",
		"configured due to new cards, the device name remains visible, but unusable.",
		"",
		"+Jumpered Cards",
		"",
		"To operate a jumpered card, first use Z to specify the card.  Then power",
		"down and install the card.",
		"",
		"+Problems",
		"",
		"If the system acts erratically, shut down, and cancel serial",
		"initialization while the system is restarting.  Then use Z to delete the",
		"device entry.  While PnP cards should cause no trouble, the I/O port",
		"of a jumpered card must be specified carefully.  Z will try to detect",
		"conflicts with other devices, but this is not guaranteed.",
		"",
		"+Serial Mouse Users",
		"",
		"On start up, the App Server looks for a serial mouse before the serial",
		"configuration is loaded.  This means the default configuration is",
		"in effect, so the mouse must be connected to /dev/ports/serial1 or",
		"/dev/ports/serial2.  That particular port must not be altered by your",
		"custom configuration.",
	};
	BFont	normal,
		bigger,
		literal;

	MoveTo( org);
	BRect br = Bounds( );
	BRect tr = br;
	tr.InsetBy( 20, 20);
	BTextView *t = new BTextView( br, 0, tr, 0, B_WILL_DRAW);
	t->SetStylable( TRUE);
	normal = be_plain_font;
	bigger = be_bold_font;
	literal = be_bold_font;
	normal.SetSize( 12);
	bigger.SetSize( 14);
	literal.SetSize( 12);
	t->SetFontAndColor( &normal);
	for (uint i=0; i<nel( info); ++i) {
		if (info[i][0] == '+') {
			if (i)
				t->Insert( "\n", 1);
			t->SetFontAndColor( &bigger);
			t->Insert( &info[i][1], strlen( &info[i][1]));
			t->SetFontAndColor( &normal);
		}
		else if (info[i][0]) {
			for (uint j=0; info[i][j]; ++j)
				if (info[i][j] == 'Z') {
					t->SetFontAndColor( &literal);
					t->Insert( "Serial", 6);
					t->SetFontAndColor( &normal);
				}
				else
					t->Insert( &info[i][j], 1);
			t->Insert( " ", 1);
		}
		else
			t->Insert( "\n", 1);
	}
	t->MakeEditable( FALSE);
	t->SetViewColor( 232, 232, 232);

	AddChild( t);
	Show( );
	Establish( 0, this, "Info");
}


void
Info::MessageReceived( BMessage *m)
{

	switch (m->what) {
	case 'acti':
		Activate( );
	}
}


Alert::Alert( char *mesg, char *button, alert_type t, bool b):
BAlert( 0, mesg, button, 0, 0, B_WIDTH_AS_USUAL, t)
{

	timeout = b;
	Go( );
}


void
Alert::Show( )
{

	if (timeout) {
		SetDestination( *this);
		Establish( 0, this);
		BTimerSend( *this, new BMessage( 'shoo'), 15000000);
	}
	BAlert::Show( );
}


void
Alert::MessageReceived( BMessage *m)
{

	if (m->what == 'shoo')
		Quit( );
	else
		BAlert::MessageReceived( m);
}


LQuery::LQuery( char *mesg, bool *r):
BAlert( 0, mesg, "Cancel", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT)
{

	*r = Go( ) < 0;
}


void
LQuery::Show( )
{

	SetDestination( *this);
	Establish( 0, this);
	BTimerSend( *this, new BMessage( 'load'), 2000000);
	BAlert::Show( );
}


void
LQuery::MessageReceived( BMessage *m)
{

	switch (m->what) {
	case 'load':
		Quit( );
		break;
	default:
		BAlert::MessageReceived( m);
	}
}


Boxed::Number::Number( Args a):
BBox( BRect( 0, 0, 400, 56))
{

	SetLabel( a.name);
	MoveTo( a.org);
	BRect r = BRect( 0, 0, 70, 22);
	t = new TV( a.text, r, 18);
	for (uint c=0; c<256; ++c)
		unless ((c)
		and (strchr( "0123456789abcdefxABCDEFX", c)))
			t->DisallowChar( c);
	t->SetAlignment( B_ALIGN_CENTER);
	t->SetWordWrap( FALSE);
	t->MoveBy( 30, 20);
	AddChild( new BScrollView( 0, t));

	SB *sb = new SB( a, this);
	AddChild( sb);
}


void
Boxed::Number::show( char *format, uint i)
{
	char	s[30];

	sprintf( s, format, i);
	t->SetText( s);
	t->Select( 99, 99);
}


Boxed::Number::SB::SB( Args a, Number *n):
Args( a), BScrollBar( BRect( 0, 0, 250, B_H_SCROLL_BAR_HEIGHT), 0, 0, 0, 0, B_HORIZONTAL)
{

	number = n;
	MoveTo( 125, 25);
}


void
Boxed::Number::SB::AttachedToWindow( )
{

	SetRange( min, max);
	SetSteps( 1, step);
	SetValue( def);
}


void
Boxed::Number::SB::ValueChanged( float v)
{

	number->show( format, v);
}


Boxed::String::String( Args a):
BBox( BRect( 0, 0, 400, 56))
{

	SetLabel( a.name);
	MoveTo( a.org);
	BRect r = BRect( 0, 0, 343, 20);
	t = new TV( a.text, r, 14);
	t->DisallowChar( B_ENTER);
	t->MoveBy( 30, 21);
	AddChild( new BScrollView( 0, t));
}


void
Boxed::String::show( char *format, uint i)
{
	char	s[30];

	sprintf( s, format, i);
	t->SetText( s);
	t->Select( 99, 99);
}


Boxed::TV::TV( char **t, BRect r, uint fs):
BTextView( r, 0, r, 0, B_WILL_DRAW|B_NAVIGABLE)
{

	text = t;
	*text = "";
	SetWordWrap( FALSE);
	BFont f = be_fixed_font;
	f.SetSize( fs);
	SetFontAndColor( &f);
}


void
Boxed::TV::InsertText( const char *a1, int32 a2, int32 a3, const text_run_array *a4)
{

	BTextView::InsertText( a1, a2, a3, a4);
	*text = Text( );
}


void
Boxed::TV::DeleteText( int32 a1, int32 a2)
{

	BTextView::DeleteText( a1, a2);
	*text = Text( );
}


void
Boxed::TV::KeyDown( const char *bytes, int32 numBytes)
{

	if (bytes[0] == B_TAB)
		BView::KeyDown( bytes, numBytes);
	else
		BTextView::KeyDown( bytes, numBytes);
}


#if 0
fmt <<'%'|sed -e 's/.*/		"&",/'
+Platforms

At this time, the Z configuration tool is for use with PC
hardware only.

+Plug-and-Play Cards

To operate a PnP serial card or modem, first
power down and install it physically.  Then use Z to identify the card.
The card will appear in the PnP list only if conflicts with other cards
(both PnP and jumpered) can be resolved.
If the card is later removed, or if it cannot be configured due to new cards,
the device name remains visible, but unusable.

+Jumpered Cards

To operate a jumpered card, first use Z to specify
the card.  Then power down and install the card.

+Problems

If the system acts erratically, shut down, and cancel
serial initialization while the system is restarting.  Then use Z to
delete the device entry.  While PnP cards should cause no trouble, the
I/O port of a jumpered card must be specified carefully.  Z will try to
detect conflicts with other devices, but this is not guaranteed.

+Serial Mouse Users

On start up, the App Server looks for a serial mouse before the serial
configuration is loaded.  This means the default
configuration is in effect, so the mouse must be connected to /dev/ports/serial1
or /dev/ports/serial2.  That particular port must not be altered by your custom
configuration.

#endif


void
Serial::A::pnpdump( )
{
	uint	i;

#if 0
	for (pnpdev *p=pnp; p; p=p->next) {
		isa_device_config *dc = &p->dc;
		printf( "card_id = 0x%X\n", dc->card_id);
		printf( "logical_device_id = 0x%X\n", dc->logical_device_id);
		printf( "card_name = \"%.*s\"\n", sizeof dc->card_name, dc->card_name);
		printf( "logical_device_name = \"%.*s\"\n", sizeof dc->logical_device_name, dc->logical_device_name);
		printf( "num_compatible_device_ids = %d\n", dc->num_compatible_device_ids);
		printf( "num_irqs = %d\n", dc->num_irqs);
		printf( "num_dma_channels = %d\n", dc->num_dma_channels);
		printf( "num_io_port_ranges = %d\n", dc->num_io_port_ranges);
		printf( "num_memory_ranges = %d\n", dc->num_memory_ranges);

		printf( "compatible device ids =");
		for (i=0; i<dc->num_compatible_device_ids; ++i)
			printf( " 0x%0X", dc->compatible_device_ids[i]);
		printf( "\n");

		printf( "irqs =");
		for (i=0; i<dc->num_irqs; ++i) {
			printf( " (%d ", dc->irqs[i].irq);
			switch (dc->irqs[i].irq_type) {
			case HTES:
				printf( "HTES");
				break;
			case LTES:
				printf( "LTES");
				break;
			case HTLS:
				printf( "HTLS");
				break;
			case LTLS:
				printf( "LTLS");
				break;
			default:
				printf( "????");
			}
			printf( ")");
		}
		printf( "\n");

		printf( "dma =");
		for (i=0; i<dc->num_dma_channels; ++i) {
			printf( " (%d ", dc->dma_channels[i].channel);
			switch (dc->dma_channels[i].flags & DMA_TYPE_MASK) {
			case DMA_TYPE_8_BIT:
				printf( " DMA_TYPE_8_BIT");
				break;
			case DMA_TYPE_8_AND_16_BIT:
				printf( " DMA_TYPE_8_AND_16_BIT");
				break;
			case DMA_TYPE_16_BIT:
				printf( " DMA_TYPE_16_BIT");
				break;
			default:
				printf( " DMA_TYPE_??");
			}

			if (dc->dma_channels[i].flags & DMA_TYPE_BUS_MASTER)
				printf( " DMA_TYPE_BUS_MASTER");
			if (dc->dma_channels[i].flags & DMA_TYPE_BYTE_MODE)
				printf( " DMA_TYPE_BYTE_MODE");
			if (dc->dma_channels[i].flags & DMA_TYPE_WORD_MODE)
				printf( " DMA_TYPE_WORD_MODE");

			switch (dc->dma_channels[i].flags & DMA_TYPE_SPEED_MASK) {
			case DMA_TYPE_SPEED_COMPAT:
				printf( " DMA_TYPE_SPEED_COMPAT");
				break;
			case DMA_TYPE_SPEED_TYPE_A:
				printf( " DMA_TYPE_SPEED_TYPE_A");
				break;
			case DMA_TYPE_SPEED_TYPE_B:
				printf( " DMA_TYPE_SPEED_TYPE_B");
				break;
			case DMA_TYPE_SPEED_TYPE_F:
				printf( " DMA_TYPE_SPEED_TYPE_F");
			}
			printf( ")");
		}
		printf( "\n");

		printf( "ports =");
		for (i=0; i<dc->num_io_port_ranges; ++i) {
			uint b = dc->io_port_ranges[i].base;
			printf( " (0x%X-0x%X)", b, b+dc->io_port_ranges[i].len-1);
		}
		printf( "\n");

		printf( "memory =");
		for (i=0; i<dc->num_memory_ranges; ++i) {
			uint b = dc->memory_ranges[i].base;
			printf( " (0x%X-0x%X)", b, b+dc->memory_ranges[i].len-1);
		}
		printf( "\n\n");
	}
#endif
}
#if 0
			if (streq( p->name, "OPTi Audio 16 / OPTi Audio 16")) {
				printf("OPTi\n");
				p->dc.card_id.id = 0x11007256;
				p->dc.logical_device_id.id = 0x2007256;
				p->dc.num_irqs = 1;
				p->dc.num_dma_channels = 0;
				p->dc.num_io_port_ranges = 1;
				p->dc.num_memory_ranges = 0;
				p->dc.io_port_ranges[0].len = 8;
				p->name = strdup( "U.S.Robotics Inc. Sportster 33. / ");
			}
#endif
