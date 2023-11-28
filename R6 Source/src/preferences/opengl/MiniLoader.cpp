#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <AppKit.h>
#include <interface/Window.h>
#include <interface/View.h>
#include <interface/Bitmap.h>
#include <game/WindowScreen.h>
#include <game/DirectWindow.h>
#include <device/graphic_driver.h>
#include <AppDefsPrivate.h>
#include <FindDirectory.h>
#include <ClassInfo.h>
#include <OS.h>
#include <Screen.h>
#include <token.h>
#include <message_util.h>
#include <session.h>
#include <messages.h>
#include <malloc.h>
#include <DirectGLWindow.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "MiniLoader.h"

#define MAX_DEVICES 16




Devices *deviceList;
int32 deviceCount = 0;

static void EnumerateDevices();

static void fixPath( char *path )
{
	char tmp[256];
	char *p = strstr( path, "_CPU_" );
	
	if( p )
	{
		strcpy( tmp, p+5 );
		strcpy( p, tmp );
		
		p = strstr( path, ".so" );
		if( p )
			p[0] = 0;

		strcpy( tmp, "_CPU_" );
		strcat( tmp, path );
		strcpy( path, tmp );
	}
}


GetAccelerantHook __glInitClone( int32 screen_index, image_id *addon_image )
{
	_BAppServerLink_ link;
	status_t result = B_ERROR;
	int32 size;
	void *clone_info;
	_direct_screen_info_ info;
	GetAccelerantHook m_gah = 0;
	
//printf( "BWindowScreen::InitClone() begins...\n" );
	// note failure by default
	*addon_image = -1;
	
	// get path to accelerant from app_server
	link.session->swrite_l(GR_GET_ACCELERANT_IMAGE_PATH);
	link.session->swrite_l(screen_index);
	link.session->flush();
	link.session->sread(sizeof(result), &result);
	if (result != B_OK)
		goto bail_out;
	link.session->sread(sizeof(size), &size);
	link.session->sread(size, info.addon_path);
	info.addon_path[size] = 0;
//printf( "info.addon_path ->%s<-\n", info.addon_path );
	
	// get the clone info from the app_server
	link.session->swrite_l(GR_GET_ACCELERANT_CLONE_INFO);
	link.session->swrite_l(screen_index);
	link.session->flush();
	link.session->sread(sizeof(result), &result);
	if (result != B_OK)
		goto bail_out;
	
	link.session->sread(sizeof(size), &size);
	clone_info = (void *)malloc(size);
	if (clone_info)
	{
		// grab the data
		link.session->sread(size, clone_info);
	}
	else
	{
		char buffer[256];
		while (size > 0)
		{
			// read out what's left
			link.session->sread(sizeof(size) > sizeof(buffer) ? sizeof(buffer) : size, buffer);
			size -= 256;
		}
		result = B_NO_MEMORY;
		goto bail_out;
	}
//printf( "got %d bytes of clone data\n", size );
	
	// load the accelerant
	*addon_image = load_add_on(info.addon_path);
	if ( (*addon_image) < 0)
	{
		result = B_ERROR;
		goto free_clone_info;
	}
	
	result = get_image_symbol(*addon_image, B_ACCELERANT_ENTRY_POINT,B_SYMBOL_TYPE_ANY,(void **)&m_gah);
	if (result != B_OK)
		goto unload_image;

	
	clone_accelerant ca;
	ca = (clone_accelerant)(m_gah)(B_CLONE_ACCELERANT, NULL);
	if (!ca)
		goto unload_image;

	result = ca(clone_info);
	if (result != B_OK)
		goto unload_image;
		
	goto free_clone_info;	
	
unload_image:
//printf( "BWindowScreen::InitClone() unloading add-on\n", result, result);
	unload_add_on(*addon_image);
	*addon_image = -1;
	m_gah = 0;
free_clone_info:
//printf( "BWindowScreen::InitClone() freeing clone info\n", result, result );
	free(clone_info);
bail_out:
//printf( "BWindowScreen::InitClone() returns %d (0x%08x)\n  ->%s\n", result, result, strerror(result));
	return m_gah;
}





#if 0
char *PathForIndex(uint32 screen_index)
{
	_BAppServerLink_ link;
	status_t result = B_ERROR;
	int32 size;
	char *path;
	
	// get device path
	link.session->swrite_l(GR_GET_DISPLAY_DEVICE_PATH);
	link.session->swrite_l(screen_index);
	// wait for app_server to send us the reply
	link.session->sync();
	link.session->sread(sizeof(result), &result);
	// if we failed, there is no more data to read
	if (result != B_OK)
		return 0;
	// on success, there is a 4 byte size
	link.session->sread(sizeof(size), &size);
	// and size bytes to follow which MUST BE READ
	path = (char *)malloc(size);
	if (path)
	{
		// grab the data
		link.session->sread(size, path);
	}
	else
	{
		char buffer[256];
		while (size > 0)
		{
			// read out what's left
			link.session->sread(sizeof(size) > sizeof(buffer) ? sizeof(buffer) : size, buffer);
			size -= sizeof(buffer);
		}
		return 0;
	}
	// return the goodies
	return path;
}
#endif



static void ScanAppServerDevices()
{
	int32 ct;
//	char *path;

	ct=0;
/*
	while( (path = PathForIndex( ct )) )
	{
		for( ct2=1; ct2<deviceCount; ct2++ )
		{
			if( ! strcmp( path, deviceList[ct2].path ) )
				deviceList[ct2].isPrimary = 1;
		}
	}
*/
	if( ct < 1 )
	{
		// We didn't find any appserver device.
		// probably means we have an old appserver
		// So we assume that the first 2d able device is the primary.
		for( int32 ct2=1; ct2<deviceCount; ct2++ )
		{
			if( deviceList[ct2].has2D )
			{
				deviceList[ct2].monitorId = BGL_MONITOR_PRIMARY;
				deviceList[ct2].isPrimary = 1;
				
				break;
			}
		}
		
	}

}


static void AddDevTree()
{
	int32 ct;
	BDirectory dir( "/dev/graphics" );
	BEntry e;
	BPath p;
	
	
	dir.Rewind();
	while( dir.GetNextEntry( &e ) >= B_OK )
	{
		e.GetPath( &p );
		if( strcmp( p.Leaf(), "stub" ) != 0 )
			deviceCount++;
	}

	deviceList = new Devices[deviceCount];
	memset( deviceList, 0, sizeof( Devices ) * deviceCount );

	// Software device
	deviceList[0].monitorId = BGL_MONITOR_PRIMARY;
	deviceList[0].path[0] = 0;
	deviceList[0].filename[0] = 0;
	deviceList[0].isPrimary = 1;
	deviceList[0].data = 0;

	ct=1;
	dir.Rewind();
	while( dir.GetNextEntry( &e ) >= B_OK )
	{
		e.GetPath( &p );
		if( strcmp( p.Leaf(), "stub" ) != 0 )
		{
			char sig[B_OS_NAME_LENGTH + 6];
			int devfd;
			if ((devfd = open( p.Path(), B_READ_WRITE )) < 0)
				continue;
			
			strcpy( deviceList[ct].path, p.Path() );
			strcpy( deviceList[ct].filename, p.Leaf() );
			deviceList[ct].monitorId = BGL_MONITOR_PRIMARY;
			deviceList[ct].isPrimary = 0;
			deviceList[ct].data = 0;

			if ( ioctl (devfd, B_GET_ACCELERANT_SIGNATURE, sig, sizeof (sig)) < B_OK )
				deviceList[ct].has2D = 0;
			else
				deviceList[ct].has2D = 1;

			if ( ioctl (devfd, B_GET_3D_SIGNATURE, sig, sizeof (sig)) < B_OK )
				deviceList[ct].has3D = 0;
			else
				deviceList[ct].has3D = 1;
			
			ct++;
			close( devfd );
		}
	}
	

}

void __glInitDeviceList()
{
	if( deviceCount )
		return;
	deviceCount = 1;
	
	AddDevTree();
	ScanAppServerDevices();
	EnumerateDevices();
}

static bool TestRequirement( uint32 req, uint32 mode )
{
	req &= BGL_BIT_DEPTH_MASK;
	if( req < BGL_8_BIT )
		return true;
	if( req != mode )
		return false;
	return true;
}


static Devices *currentDevice;

static void EnumModeCallback( video_mode *mode )
{
	ConfigMode *cfg = new ConfigMode;
	
	if( (!mode->width) || (!mode->height) )
		return;
	
	cfg->w = mode->width;
	cfg->h = mode->height;
	cfg->hz = mode->refresh;
	
	switch( mode->color & BGL_BIT_DEPTH_MASK )
	{
		case BGL_8_BIT: cfg->bpp=8; break;
		case BGL_15_BIT: cfg->bpp=15; break;
		case BGL_16_BIT: cfg->bpp=16; break;
		case BGL_24_BIT: cfg->bpp=24; break;
		case BGL_32_BIT: cfg->bpp=32; break;
		case BGL_48_BIT: cfg->bpp=48; break;
		case BGL_64_BIT: cfg->bpp=64; break;
		case BGL_128_BIT: cfg->bpp=128; break;
	}
	
	cfg->next = 0;
	cfg->enabled = 0;
	
	if( currentDevice->firstMode )
	{
		ConfigMode *prev = currentDevice->firstMode;
		while( prev->next )
			prev = prev->next;
		prev->next = cfg;
	}
	else
	{
		currentDevice->firstMode = cfg;
	}
	
}

static void LoadDeviceConfig( Devices *dev )
{
	BPath p;
	FILE *f;
	status_t result;
	
	result = find_directory(B_USER_SETTINGS_DIRECTORY, &p);
	if( result < B_OK )
		return;
	
	result = p.Append( "opengl" );
	if( result < B_OK )
		return;

	result = p.Append( dev->filename );
	if( result < B_OK )
		return;
		
	f = fopen( p.Path(), "rb" );
	if( f )
	{
		char buf[256];
		
		ConfigMode *cm = dev->firstMode;
		while( cm )
		{
			cm->enabled = 0;
			cm = cm->next;
		}
		
		while( fgets( buf, 255, f ) )
		{
			char token[64];
			sscanf( buf, "%s", token );
			
			if( !strcmp( token, "res" ) )
			{
				int w, h, hz;
				if( sscanf( buf, "res %ix%i %i", &w, &h, &hz ) == 3 )
				{
					cm = dev->firstMode;
					while( cm )
					{
						if( (cm->w==w) && (cm->h==h) && (cm->hz==hz) )
							cm->enabled = 1;
						cm = cm->next;
					}
				}
			}

			if( !strcmp( token, "gamma" ) )
			{
				if( sscanf( buf, "gamma %f,%f,%f", &dev->gamma[0], &dev->gamma[1], &dev->gamma[2] ) != 3 )
				{
					// Not in config file so use defaults
					dev->gamma[0] = 1.7;
					dev->gamma[1] = 1.7;
					dev->gamma[2] = 1.7;
				}
			}

			if( !strcmp( token, "monitor" ) )
			{
				if( sscanf( buf, "monitor %i", &dev->monitorId) != 1 )
				{
					// Not in config file so use defaults
					dev->monitorId = BGL_MONITOR_PRIMARY;
				}
			}
			
	
		}
		fclose( f );
	}
	else
	{
		// Ok lets add some intelligent defaults here
		ConfigMode *cm = dev->firstMode;
		while( cm )
		{
			cm->enabled = (cm->hz==60);
			cm = cm->next;
		}
		dev->gamma[0] = 1.7;
		dev->gamma[1] = 1.7;
		dev->gamma[2] = 1.7;
		dev->monitorId = 0;
		
	}
	
}

static void EnumerateDevices()
{
	static BLocker lock;
	lock.Lock();

	int32 ct;
	for( ct=1; ct<deviceCount; ct++ )
	{
		currentDevice = &deviceList[ct];

		if( deviceList[ct].isPrimary )
		{
			//App Server device
			image_id id;
			GetAccelerantHook gah = __glInitClone(0, &id);

			if( gah )
			{
				// Get extra hooks here
				_3D_get_device_info getDeviceInfo = (_3D_get_device_info)(gah)(B_3D_GET_DEVICE_INFO, NULL);
				_3D_get_modes getModes = (_3D_get_modes)(gah)(B_3D_GET_MODES, NULL);

				if( getDeviceInfo && getModes )
				{
					const char *name;
					uint8 depth, stencil, accum;
					(getDeviceInfo) ( deviceList[ct].path, &name, &depth, &stencil, &accum );
					strcpy( deviceList[ct].name, name );
					(getModes) ( deviceList[ct].path, EnumModeCallback, BGL_ANY, BGL_NONE, BGL_NONE, BGL_NONE );
				}

				uninit_accelerant uninit = (uninit_accelerant)(gah)(B_UNINIT_ACCELERANT, NULL);
				if( uninit )
					(uninit)();
			}
			
			if( id >= B_OK )
			{
				unload_add_on( id );
			}
		}
		else
		{
			int fd;
			// Secondary device here
			// Do a load_addon here
			fd = open(deviceList[ct].path, O_RDWR);
			if (fd < 0)
			{
				printf("EnumerateDevices - error opening %s\n", deviceList[ct].path);
			}
			else
			{
				char addon_name[256];
				char *p;
				image_id id;

				strcpy(addon_name,"accelerants/");
				p = addon_name + strlen(addon_name);
				ioctl(fd, B_GET_3D_SIGNATURE, p, 256);

				fixPath( addon_name );
				id = load_add_on(addon_name);

				if (id >= B_OK)
				{
					GetAccelerantHook gah;
					if (get_image_symbol(id, "get_accelerant_hook", B_SYMBOL_TYPE_TEXT, (void **)&gah) == B_NO_ERROR)
					{
						_3D_get_device_info getDeviceInfo = (_3D_get_device_info)(gah)(B_3D_GET_DEVICE_INFO, NULL);
						_3D_get_modes getModes = (_3D_get_modes)(gah)(B_3D_GET_MODES, NULL);
		
						if( getDeviceInfo && getModes )
						{
							const char *name;
							uint8 depth, stencil, accum;
							(getDeviceInfo) ( deviceList[ct].path, &name, &depth, &stencil, &accum );
							strcpy( deviceList[ct].name, name );
							(getModes) ( deviceList[ct].path, EnumModeCallback, BGL_NONE, BGL_NONE, BGL_NONE, BGL_NONE );
						}
					}
					unload_add_on(id);
				}
				close(fd);
			}
		}
		LoadDeviceConfig( currentDevice );
	}

	lock.Unlock();
}



