#ifndef _DC10_DRIVER_H
#define _DC10_DRIVER_H

#include <Drivers.h>
#include <PCI.h>
#include <GraphicsDefs.h>


#define DRIVER_VERSION	4
#define DEVICE_NAME		"video/dc10/"
#define MAX_CARDS 4
#define MAX_BUFFER 6

#define DC10_CAPTURE_OK			0
#define	DC10_CAPTURE_TIMEOUT		1

//ioctl command
enum {
    DC10_GET_INFO = B_DEVICE_OP_CODES_END + 1,
    DC10_SYNC,
    DC10_RESET_FRAME_NUMBER,
    DC10_SET_MODE,
    DC10_GET_MODE,
    DC10_GET_BUFFER_INFO,
    DC10_SET_BUFFER,
    DC10_WRITE_INDEX};

typedef struct {
	int32 	blocked_threads;
	sem_id	sem;
	uint32	event_count;
} event_t;

//struct of one board
typedef struct {
		//PCI config
		int id;
        int index; 
        uchar bus;
        uchar device;
        uchar function;
        uchar size;
        uchar offset;
        uint32 value;
        uint32 interruptline;
        event_t	sync_event;			// capture complete 

   
   		//specific register config of zoran36067
        char  reg_name[256];
        void *reg_physical_address;
        size_t reg_size;
        uint32 reg_flags;
        uint32 reg_protection;
        void *reg_address;
        area_id reg_area_id;
      
  		void *buffer_physical_address;
        void *buffer_address;
        area_id buffer_area;
        uint32 buffer_size;
        uint32	buffer_index;
        
        uint32			frame_number;
		bigtime_t		time_stamp;
		bigtime_t		time_start;
		uint32			capture_status;
		
		bigtime_t		timeout;		// timeout for SYNC ioctl 
		
        uint32 			mode;
       
        
        
} dc10_private_config;

//struct of one board
typedef struct {
		int32 fd;
        void *reg_address;
        
        uint32 decoder_ad;
        uint32 encoder_ad;
        
  		void *buffer_physical_address;
  		void * buffer;
    	uint32 buffer_info[4];
    	uint32 buffer_reset_index;
    	uint32 buffer_index;
    	
      	uint32 buffer_size;
        
        uint32			frame_number;
		bigtime_t		time_stamp;
		bigtime_t		time_start;
		uint32			capture_status;
		
		bigtime_t		timeout;		// timeout for SYNC ioctl 
		
        uint32 			mode;
        bool mEnabled;
        uint32 video_format;
       
        
        
} dc10_config;


//struct of all dc10+ on board
typedef struct {
        int number;
        int index[MAX_CARDS]; 
        uchar bus[MAX_CARDS];
        uchar device[MAX_CARDS];
        uchar function[MAX_CARDS];
} dc10_cards;

#endif
