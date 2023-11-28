#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <recovery/factory_settings.h>


static
factory_settings_t *
create_empty_settings(uint32 size)
{
    factory_settings_t        *retVal= malloc(sizeof(factory_settings_t));
    factory_settings_buffer_t *buf;

    retVal->size= size;
    retVal->buffer= malloc(size);
    
    buf= (factory_settings_buffer_t *)retVal->buffer;
    buf->magik= ZR_FACTORY_MAGIC;
    buf->size = size;
    buf->used = sizeof(factory_settings_buffer_t);

    return retVal;
}

factory_settings_t *
create_settings_from_file(char const *fname, uint32 size)
{
    FILE *f;
    char  buffer[1024];

    factory_settings_t        *retVal= create_empty_settings(size);
    factory_settings_buffer_t *work  = (factory_settings_buffer_t *)retVal->buffer;

    if(fname) {
        f= fopen(fname, "r");
    } else {
        f= stdin;
    }

    if(!f) {
        perror(fname);
        exit(1);
    }
    
    while(fgets(buffer, sizeof(buffer), f)) {
        uint32 len;
        
        while(strchr(buffer, '\n')) {
            strchr(buffer, '\n')[0]= 0;
        }
        if(buffer[0]== '#') {
            continue;
        }
        if(strlen(buffer)== 0) {
            continue;
        }

        len= 1+strlen(buffer);

        if(work->used+sizeof(len)+len> work->size) {
            fprintf(stderr, "factory settings area overflown\n");
            exit(1);
        }
        
        memcpy(retVal->buffer+work->used, &len, sizeof(len));
        work->used+= sizeof(len);
        memcpy(retVal->buffer+work->used, buffer, len);
        work->used+= len;
        
        printf("Adding: %s\n", buffer);
    }

    return retVal;
}
