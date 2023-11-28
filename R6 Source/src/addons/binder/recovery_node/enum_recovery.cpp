/*
 * enum_recovery.hh -- enumerator for zrecover config
 *
 * $au$: manuel<mpetit@be.com> -- Sun Oct 15 16:06:41 PDT 2000
 * $ww$: ts=4
 *
 * $ed$: manuel<mpetit@be.com> -- Sun Oct 15 16:06:41 PDT 2000
 *
 *
 * Copyright 2000, Be Incorporated, All Rights Reserved.
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include <os_p/priv_syscalls.h>
#include <drivers/Drivers.h>
#include <recovery/factory_settings.h>
#include "enum_recovery.h"


static unsigned const BOOTSECTOR_SIZE= 512;
static unsigned const MAX_SETTINGS= 512;


zrecover_config_item_t::zrecover_config_item_t(char const *key, char const *value, unsigned kind)
{
    this->key  = strdup(key);
	this->value= strdup(value);
	this->kind = kind_e(kind);
}

zrecover_config_item_t::~zrecover_config_item_t(void)
{
    free(key)  ; key  = NULL;
	free(value); value= NULL;
}
	
zrecover_config_item_t *
new_config_item(char const *key, char const *value, unsigned kind)
{
    return new zrecover_config_item_t(key, value, kind);
}





//
//
//
struct zrecover_config_internal_t
{
private:
    bool  found_settings;
    int   zrecover_fd;
    off_t zrecover_offset;
    off_t zrecover_length;
    off_t settings_offset;
    off_t settings_length;

    factory_settings_buffer_t *on_disk_settings;

    unsigned                 max_settings;
    unsigned                 used_settings;
    zrecover_config_item_t **settings;

    status_t decode_factory_settings(void);
    status_t find_settings(void);

public:
    zrecover_config_internal_t(void);
	zrecover_config_internal_t(char const *path);

	virtual ~zrecover_config_internal_t(void);

	unsigned                      CountItems(void) const;
	zrecover_config_item_t const *ItemAt(unsigned) const;


    status_t Configure(char const *key, char const *value);
    status_t ZapConfiguration(void);

	status_t Cancel(void);
	status_t Commit(void);
};

status_t
zrecover_config_internal_t::decode_factory_settings(void)
{
    uint32   max_bytes= on_disk_settings->used;
    uint32   got_bytes= sizeof(factory_settings_buffer_t);

    uchar   *c_ptr= ((uchar*)on_disk_settings)+got_bytes;
    uint32  *i_ptr= (uint32*)c_ptr;

    while(got_bytes+sizeof(uint32)+i_ptr[0] <= max_bytes) {
        c_ptr+= sizeof(uint32);

        char *key;
        char *value;
        char *line= new char [i_ptr[0]];

        memcpy(line, c_ptr, i_ptr[0]);

        if(!strchr(line, '=')) {
            continue;
        }

        key= line;
        value= strchr(line, '=');

        value[0]= 0;
        value++;

        settings[used_settings]= new_config_item(key, value, zrecover_config_item_t::ZR_SETTING);
        used_settings++;

        got_bytes+= sizeof(uint32)+i_ptr[0];
        c_ptr+= i_ptr[0];
        i_ptr= (uint32*)c_ptr;

		delete [] line;
    }

    return B_OK;
}


status_t
zrecover_config_internal_t::find_settings(void)
{
    int    i;

    uchar sect[BOOTSECTOR_SIZE];
    uchar mbr[BOOTSECTOR_SIZE];
    struct partition_entry {
        uchar   active;
        uchar   start_chs[3];
        uchar   id;
        uchar   end_chs[3];
        uint32  start_lba;
        uint32  len_lba;
    } *pent= (struct partition_entry *)(mbr+0x1be);

    if(zrecover_fd< 0) {
        return B_ERROR;
    }


    /*
     * fetch the MBR
     */
    if(read_pos(zrecover_fd, 0LL, mbr, sizeof(mbr))!= sizeof(mbr)) {
        return B_ERROR;
    }

    /*
     * query second partition
     */
    if(pent[1].id!= 0xeb) {
        fprintf(stderr, "Recovery partition not found\n");
        return B_ERROR;
    }

    zrecover_offset= pent[1].start_lba;
    zrecover_length= pent[1].len_lba;


    /*
     * verify the recovery image
     */
    if(read_pos(zrecover_fd, BOOTSECTOR_SIZE*zrecover_offset, sect, sizeof(sect))!= sizeof(sect)) {
        fprintf(stderr, "Recovery partition not found or corrupted\n");
        return B_ERROR;
    }
    if((sect[0x00]!= 0xeb) || (sect[BOOTSECTOR_SIZE-2]!= 0x55) || (sect[BOOTSECTOR_SIZE-1]!= 0xaa)) {
        fprintf(
            stderr,
            "Recovery partition test 1 failed %02x %02x %02x\n",
            sect[0x00],
            sect[BOOTSECTOR_SIZE-2],
            sect[BOOTSECTOR_SIZE-1]
        );
        return B_ERROR;
    }
    if(((unsigned short*)(sect+0x1f0))[0]!= zrecover_length) {
        fprintf(stderr, "Size mismatch between zrecover and partition table\n");
        return B_ERROR;
    }


    /*
     * try to locate the old factory settings
     */
    settings_offset= 0;
    settings_length= 0;
    for(i= 1; i< zrecover_length; i++) {
        factory_settings_buffer_t *aux= (factory_settings_buffer_t*)(sect);

        if(read_pos(zrecover_fd, BOOTSECTOR_SIZE*(zrecover_offset+i), sect, sizeof(sect))!= sizeof(sect)) {
            fprintf(stderr, "Read error while searching for settings\n");
            return B_ERROR;
        }

        if(aux->magik!= ZR_FACTORY_MAGIC) {
            continue;
        }
        if(aux->size/BOOTSECTOR_SIZE+i!= zrecover_length) {
            continue;
        }
        if(aux->used> aux->size) {
            continue;
        }

        settings_offset= i;
        settings_length= aux->size;
    }


    if(!settings_length) {
        fprintf(stderr, "Factory settings reserved area not found\n");
        return B_ERROR;
    }

    on_disk_settings= reinterpret_cast<factory_settings_buffer_t*>(new char [settings_length]);

    if(read_pos(zrecover_fd, BOOTSECTOR_SIZE*(zrecover_offset+settings_offset), on_disk_settings, settings_length)!= settings_length) {
	    delete on_disk_settings;
        on_disk_settings= NULL;
		return B_ERROR;
	}


    return decode_factory_settings();
}

zrecover_config_internal_t::zrecover_config_internal_t(void)
: found_settings(false),
  zrecover_fd(-1),
  zrecover_offset(-1),
  zrecover_length(-1),
  settings_offset(-1),
  settings_length(-1),
  on_disk_settings(NULL),
  max_settings(MAX_SETTINGS),
  used_settings(0),
  settings(new zrecover_config_item_t * [MAX_SETTINGS])
{
    /*
     * most of this is ripped from bootcomplete
     *
     */
    status_t err;
    fs_info info;
    int fd;
    partition_info pi;

    err= _kstatfs_(-1, NULL, -1, "/boot", &info);
    if(err != B_NO_ERROR) {
        fprintf(stderr, "can't find boot device, %s\n", strerror(err));
        return;
    }

    fd= open(info.device_name, O_RDONLY);
    if(fd< 0) {
        fprintf(stderr, "can't open boot device, %s\n", strerror(errno));
        return;
    }

    if(ioctl(fd, B_GET_PARTITION_INFO, &pi) < 0) {
        fprintf(stderr, "can't get partition info for boot device, %s\n", strerror(fd));
        close(fd);
        return;
    }
    close(fd);

    zrecover_fd= open(pi.device, O_RDWR);

    find_settings();
}

zrecover_config_internal_t::zrecover_config_internal_t(char const *path)
: found_settings(false),
  zrecover_fd(-1),
  zrecover_offset(-1),
  zrecover_length(-1),
  settings_offset(-1),
  settings_length(-1),
  on_disk_settings(NULL),
  max_settings(MAX_SETTINGS),
  used_settings(0),
  settings(new zrecover_config_item_t * [MAX_SETTINGS])
{
    zrecover_fd= open(path, O_RDWR, 000);

    find_settings();
}

zrecover_config_internal_t::~zrecover_config_internal_t(void)
{
    close(zrecover_fd); zrecover_fd= -1;

    delete [] (reinterpret_cast<char*>(on_disk_settings)); on_disk_settings= NULL;

    for(unsigned i= 0; i< used_settings; i++) {
	    delete settings[i]; settings[i]= NULL;
	}
	delete [] settings;
}

unsigned
zrecover_config_internal_t::CountItems(void) const
{
    return used_settings;
}

zrecover_config_item_t const *
zrecover_config_internal_t::ItemAt(unsigned idx) const
{
    if(idx>= used_settings) {
        return NULL;
    }

    return settings[idx];
}

status_t
zrecover_config_internal_t::Configure(char const *key, char const *value)
{
    for(unsigned i= 0; i< used_settings; i++) {
        if(strcmp(settings[i]->Key(), key)== 0) {
		    zrecover_config_item_t *old= settings[i];

            settings[i]= new_config_item(key, value, zrecover_config_item_t::ZR_SETTING);

            delete old;
            return B_OK;
        }
    }

    settings[used_settings]= new_config_item(key, value, zrecover_config_item_t::ZR_SETTING);
    used_settings++;

    return B_OK;
}

status_t
zrecover_config_internal_t::ZapConfiguration(void)
{
    for(unsigned i= 0; i< used_settings; i++) {
        delete settings[i];
        settings[i]= NULL;
    }

    used_settings= 0;

    return B_OK;
}

status_t
zrecover_config_internal_t::Cancel(void)
{
    ZapConfiguration();
    decode_factory_settings();

    return B_OK;
}

status_t
zrecover_config_internal_t::Commit(void)
{
    on_disk_settings->used= sizeof(factory_settings_buffer_t);

    for(unsigned i= 0; i< used_settings; i++) {
        uint32  len= strlen(settings[i]->Key())+1+strlen(settings[i]->Value())+1;
        char   *buf= new char [len];

        sprintf(buf, "%s=%s", settings[i]->Key(), settings[i]->Value());

        len= 1+strlen(buf);

        if(on_disk_settings->used+sizeof(len)+len> on_disk_settings->size) {
            fprintf(stderr, "factory settings area overflown\n");
            return B_ERROR;
        }

        memcpy((reinterpret_cast<char*>(on_disk_settings))+on_disk_settings->used, &len, sizeof(len));
        on_disk_settings->used+= sizeof(len);
        memcpy((reinterpret_cast<char*>(on_disk_settings))+on_disk_settings->used, buf, len);
        on_disk_settings->used+= len;

        printf("Adding: %s\n", buf);
    }

    if(write_pos(zrecover_fd, BOOTSECTOR_SIZE*(zrecover_offset+settings_offset), on_disk_settings, settings_length)!= settings_length) {
        return B_ERROR;
    }

    return B_OK;
}



//
//
//
zrecover_config_t::zrecover_config_t(void)
{
    hidden= new zrecover_config_internal_t();
}

zrecover_config_t::zrecover_config_t(char const *path)
{
    hidden= new zrecover_config_internal_t(path);
}

zrecover_config_t::~zrecover_config_t(void)
{
    delete (reinterpret_cast<zrecover_config_internal_t*>(hidden));
}

unsigned
zrecover_config_t::CountItems(void) const
{
    return reinterpret_cast<zrecover_config_internal_t*>(hidden)->CountItems();
}

zrecover_config_item_t const *
zrecover_config_t::ItemAt(unsigned idx) const
{
    return reinterpret_cast<zrecover_config_internal_t*>(hidden)->ItemAt(idx);
}


status_t
zrecover_config_t::Configure(char const *key, char const *value)
{
    return reinterpret_cast<zrecover_config_internal_t*>(hidden)->Configure(key, value);
}

status_t
zrecover_config_t::ZapConfiguration(void)
{
    return reinterpret_cast<zrecover_config_internal_t*>(hidden)->ZapConfiguration();
}

status_t
zrecover_config_t::Cancel(void)
{
    return reinterpret_cast<zrecover_config_internal_t*>(hidden)->Cancel();
}

status_t
zrecover_config_t::Commit(void)
{
    return reinterpret_cast<zrecover_config_internal_t*>(hidden)->Commit();
}



#ifdef DEBUG
int
main(int argc, char **argv)
{
    if(argc!= 3) {
	    return 1;
	}

	zrecover_config_t zr(argv[1]);

	printf("items= %d\n", zr.CountItems());

	for(unsigned i= 0; i< zr.CountItems(); i++) {
	    printf("[%s,%s]\n", zr.ItemAt(i)->Key(), zr.ItemAt(i)->Value());
	}

    printf("\n\n\nconfiguring...\n");

	for(unsigned i= 0; i< zr.CountItems(); i++) {
	    zr.Configure(zr.ItemAt(i)->Key(), argv[2]);
	}

	for(unsigned i= 0; i< zr.CountItems(); i++) {
	    printf("[%s,%s]\n", zr.ItemAt(i)->Key(), zr.ItemAt(i)->Value());
	}

    printf("\n\n\ncanceling...\n");

    zr.Cancel();

	for(unsigned i= 0; i< zr.CountItems(); i++) {
	    printf("[%s,%s]\n", zr.ItemAt(i)->Key(), zr.ItemAt(i)->Value());
	}

    printf("\n\n\nconfiguring & commit...\n");

	for(unsigned i= 0; i< zr.CountItems(); i++) {
	    zr.Configure(zr.ItemAt(i)->Key(), argv[2]);
	}

	for(unsigned i= 0; i< zr.CountItems(); i++) {
	    printf("[%s,%s]\n", zr.ItemAt(i)->Key(), zr.ItemAt(i)->Value());
	}

	zr.Commit();

}
#endif
