/*
   	Public interfaces to the BT848 driver (device control opcodes, structs, etc).
*/

#ifndef BT848_DRIVER_H
#define BT848_DRIVER_H

#include <OS.h>
#include <Drivers.h>

/* -----
	ioctl opcodes
----- */

enum {
	BT848_INIT_REGS = B_DEVICE_OP_CODES_END + 1,	
		/* initialize Bt848 registers								 	*/
		/* required input parameters (Bt848_config struct):				*/
		/* video_source, video_format, color_format, x_size, y_size,	*/
		/* brightness, contrast, hue, saturation, capture_mode,			*/
		/* decimate														*/
		/* return values:												*/
		/* none															*/
	BT848_BUILD_PRI,		//1
		/* build primary risc program									*/
		/* does not reinitialize Bt848 registers						*/
		/* required input parameters (Bt848_config struct):				*/
		/* video_source, video_format, color_format, x_size, y_size,	*/
		/* capture_mode, e_address_type, o_address_type, e_line, 		*/
		/* o_line, e_clip, o_clip, o_endian, e_endian, o_vbi, e_vbi		*/
		/* number_buffers												*/
		/* return values:												*/
		/* none															*/
	BT848_BUILD_ALT,		//2
		/* build alternate risc program 								*/
		/* does not reinitialize Bt848 registers						*/
		/* required input parameters (Bt848_config struct):				*/
		/* video_source, video_format, color_format, x_size, y_size,	*/
		/* capture_mode, e_address_type, o_address_type, e_line, 		*/
		/* o_line,e_clip, o_clip, o_endian, e_endian, o_vbi, e_vbi		*/
		/* number_buffers												*/
		/* return values:												*/
		/* none															*/
	BT848_START_PRI,		//3
		/* starts video capture using primary risc program				*/
		/* required input parameters (Bt848_config struct):				*/
		/* number_buffers												*/
		/* return values:												*/
		/* none															*/
	BT848_START_ALT,		//4
		/* starts video capture using alternate risc program			*/
		/* required input parameters (Bt848_config struct):				*/
		/* number_buffers												*/
		/* return values:												*/
		/* frame_count set to 0											*/
	BT848_RESTART_PRI,		//5
		/* starts video capture using alternate risc program			*/
		/* frame_count not modified										*/
		/* required input parameters (Bt848_config struct):				*/
		/* none															*/
		/* return values:												*/
		/* none															*/
	BT848_RESTART_ALT,		//6
		/* starts video capture using alternate risc program			*/
		/* frame_count not modified										*/
		/* required input parameters (Bt848_config struct):				*/
		/* none															*/
		/* return values:												*/
		/* none															*/
	BT848_SWITCH_PRI,		//7
		/* switches to primary risc program on the next frame			*/
		/* required input parameters (Bt848_config struct):				*/
		/* none															*/
		/* return values:												*/
		/* none															*/
	BT848_SWITCH_ALT,		//8
		/* switches to alternate risc program on the next frame			*/
		/* required input parameters (Bt848_config struct):				*/
		/* none															*/
		/* return values:												*/
		/* none															*/
	BT848_STOP,				//9
		/* stops video capture											*/
		/* required input parameters (Bt848_config struct):				*/
		/* none															*/
		/* return values:												*/
		/* none															*/
	BT848_GET_LAST,			//10
		/* returns last captured frame									*/
		/* required input parameters (Bt848_config struct):				*/
		/* none															*/
		/* return values:												*/
		/* last_buffer, frame_number, time_stamp, capture_status		*/
	BT848_GET_NEXT,			//11
	BT848_SYNC,				//12
		/* blocks until capture of next frame							*/
		/* required input parameters (Bt848_config struct):				*/
		/* none															*/
		/* return values:												*/
		/* last_buffer, frame_number, time_stamp, capture_status		*/
	BT848_GET_NEXT_TIMEOUT,	//13
	BT848_SYNC_TIMEOUT,		//14
		/* blocks until capture of next frame or timeout				*/
		/* required input parameters (Bt848_config struct):				*/
		/* timeout (in microseconds)									*/
		/* return values:												*/
		/* last_buffer, frame_number, time_stamp, capture_status		*/
	BT848_SELECT_VIDEO_SOURCE,//15
		/* selects specified video source								*/
		/* required input parameters (Bt848_config struct):				*/
		/* video_source,video_format									*/
		/* return values:												*/
		/* none															*/
	BT848_BRIGHTNESS,		//16
		/* adjusts brightness											*/
		/* required input parameters (Bt848_config struct):				*/
		/* brightness													*/
		/* return values:												*/
		/* none															*/
	BT848_CONTRAST,			//17
		/* adjusts contrast												*/
		/* required input parameters (Bt848_config struct):				*/
		/* contrast														*/
		/* return values:												*/
		/* none															*/
	BT848_HUE,				//18
		/* adjusts hue													*/
		/* required input parameters (Bt848_config struct):				*/
		/* hue															*/
		/* return values:												*/
		/* none															*/
	BT848_SATURATION,		//19
		/* adjusts saturation											*/
		/* required input parameters (Bt848_config struct):				*/
		/* saturation													*/
		/* return values:												*/
		/* none															*/
	BT848_ALLOCATE_BUFFER,	//20
		/* allocates locked contiguous buffer for video capture			*/
		/* required input parameters (Bt848_buffer struct):				*/
		/* buffer_size													*/
		/* return values:												*/
		/* buffer_address, buffer_area									*/
	BT848_FREE_BUFFER,		//21
		/* frees buffer allocated by BT848_ALLOCATE_BUFFER				*/
		/* required input parameters:
		/* Bt848_buffer struct											*/
		/* return values:												*/
		/* none															*/
	BT848_I2CREAD,			//22
		/* reads specified I2C address									*/
		/* required input parameters (Bt848_config struct):				*/
		/* i2c_address													*/
		/* return values:												*/
		/* i2c_data1,i2c_status											*/
	BT848_I2CWRITE1,		//23
		/* writes 1 byte to specified I2C address						*/
		/* required input parameters (Bt848_config struct):				*/
		/* i2c_address, i2c_data1										*/
		/* return values:												*/
		/* i2c_status													*/
	BT848_I2CWRITE2,		//24
		/* writes 2 bytes to specified I2C address						*/
		/* required input parameters (Bt848_config struct):				*/
		/* i2c_address, i2c_data1, i2c_data2							*/
		/* return values:												*/
		/* i2c_status													*/
		
	BT848_WRITE_GPIO_OUT_EN,//25
	BT848_WRITE_GPIO_IN_EN,	//26
	BT848_READ_GPIO_DATA,	//27
	BT848_WRITE_GPIO_DATA,	//28
	
	BT848_STATUS,			//29
		/* returns Bt848 status register								*/
		/* required input parameters (Bt848_config struct):				*/
		/* none															*/
		/* return values:												*/
		/* status														*/
	BT848_GAMMA,			//30
		/* enables/disables gamma correction *removal*					*/
		/* required input parameters (Bt848_config struct):				*/
		/* gamma														*/
		/* return values:												*/
		/* none															*/
	BT848_ERROR_DIFFUSION,	//31
		/* enables/diables error diffusion in RGB16/15 modes			*/
		/* required input parameters (Bt848_config struct):				*/
		/* error_diffusion												*/
		/* return values:												*/
		/* none															*/
	BT848_LUMA_CORING,		//32
		/* enables/disables luma coring									*/
		/* required input parameters (Bt848_config struct):				*/
		/* luma_coring													*/
		/* return values:												*/
		/* none															*/
	BT848_LUMA_COMB,
		/* enables/disables luma comb filter								*/
		/* required input parameters (Bt848_config struct):				*/
		/* luma_comb													*/
		/* return values:												*/
		/* none															*/
	BT848_CHROMA_COMB,
		/* enables/disables chroma comb filter							*/
		/* required input parameters (Bt848_config struct):				*/
		/* chroma_comb													*/
		/* return values:												*/
		/* none															*/
	BT848_COLOR_BARS,
		/* enables/disables color bars									*/
		/* required input parameters (Bt848_config struct):				*/
		/* color_bars													*/
		/* return values:												*/
		/* none															*/
	BT848_READ_I2C_REG,
		/* reads I2C Data/Control Register								*/
		/* required input parameters (Bt848_config struct):				*/
		/* none															*/
		/* return values:												*/
		/* i2c_register													*/
	BT848_WRITE_I2C_REG,
		/* writes I2C Data/Control Register								*/
		/* required input parameters (Bt848_config struct):				*/
		/* i2c_register													*/
		/* return values:												*/
		/* none															*/
	BT848_VERSION,
		/* returns driver version & chip id								*/
		/* return value:												*/
		/* version, device_id											*/
	BT848_PLL
		/* enables/disables phase locked loop							*/
		/* required input parameters (Bt848_config struct):				*/
		/* pll															*/
		/* return values:												*/
		/* none															*/
	};

/*
	Bt848 typedefs
*/

typedef 
	int16				*bt848_cliplist[576];

typedef struct {
	uint32				status;			/* status return variable --see defines below	*/
	
	uint16				video_source;	/* video source -- see defines below			*/
	uint16				decimate;		/* number of frames per second to skip			*/

	uint16				video_format;	/* video format -- see defines below)			*/
	uint16				capture_mode;	/* continuous or single frame/field capture	*/


	uint16				o_color_format;	/* odd lines color format  -- see defines below)	*/
	uint16				e_color_format;	/* even lines color format  -- see defines below)	*/

	uint16				o_x_size;		/* odd lines scaled video image width in pixels 	*/
	uint16				e_x_size;		/* even lines scaled video image width in pixels 	*/

	uint16				o_y_size;		/* odd lines scaled video image height in pixels	*/
	uint16				e_y_size;		/* even lines scaled video image height in pixels	*/
	
	uint16				o_address_type;	/* indicates whether the addresses in o_line are logical or physical */
										/* physical addresses are PCI addresses (ie use ramaddress()) */
	uint16				e_address_type;	/* logical or physical addresses in e_line */
										/* physical addresses are PCI addresses (ie use ramaddress()) */
										
	uint16				o_endian;		/* set to desired endianess for odd field capture		*/
	uint16				e_endian;		/* set to desired endianess for even field capture		*/
	
	void		 		***o_line;		/* pointer to an array of pointers to arrays of pointers to storage for odd field scan lines */
										/* set this pointer to NULL to disable capture of odd field */
	void		 		***e_line;		/* pointer to an array of pointers to arrays of pointers to storage for even field scan lines */
										/* set this pointer to NULL to disable capture of even field */
	bt848_cliplist		*o_clip;		/* pointer to array of pointers to clip list 			*/
										/* clip list is an null terminated arrary of int runs 	*/
										/* positive values for number of pixels to display    	*/
										/* negative values for number of pixels to skip       	*/ 
										/* set this pointer to null to disable clipping 		*/
	bt848_cliplist		*e_clip;		/* pointer to array of pointers to clip list 			*/
										/* clip list is an null terminated arrary of int runs 	*/
										/* positive values for number of pixels to display    	*/
										/* negative values for number of pixels to skip       	*/ 
										/* set this pointer to null to disable clipping 			*/
	void				***o_vbi;		/* pointer to an array of pointers for odd field vbi data	*/
	void				***e_vbi;		/* pointer to an array of pointers for even field vbi data	*/

	uint32				number_buffers;	/* number of buffers in ring */
	uint32				current_buffer;	/* index to current buffer in ring */
	uint32				last_buffer;	/* index to most recently filled buffer */
	
	bigtime_t			timeout;		/* timeout for SYNC ioctl */
	
	uint32				frame_number;	/* frame count since START ioctl  */
	bigtime_t			time_stamp;		/* system time for captured frame */ 
	uint32				capture_status;	/* zero for successful capture */
	uint32				i2c_register;	/* i2c data/control register */
		
	uint32				gpio_out_en;	/* output buffer enables for gpio, 1 enables the output		*/
	uint32				gpio_in_en;		/* input mux selection for gpio, 0 = direct, 1 = registered	*/
	uint32				gpio_data;		/* gpio data													*/	

	uchar				i2c_address;	/* address for I2C transactions */
	uchar				i2c_data1;		/* first data byte for I2C transactions */
	uchar				i2c_data2;		/* second data byte for I2C transactions */
	char				i2c_status;		/* return status from I2C read or write  */

	char				brightness;		/* brightness control, default: 0, -100 to +100	*/
	char				contrast;		/* odd lines contrast control, default:0, -100 to +100		*/
	char				hue;			/* even lines hue adjustment, default: 0, -100 to +100			*/
	char				saturation;		/* even lines saturation adjustment, default: 0, -100 to +100	*/
	
	bool				gamma;			/* enable/disable gamma correction *removal* */
	bool				error_diffusion;/* enable/disable error diffusion in rgb16/15 */
	bool				luma_coring;	/* enable/disable luma coring for luminance < 16 */
	bool				luma_comb;		/* enable/disable luma comb filter */
	bool				chroma_comb;	/* enable/disable chroma comb filter */
	bool				color_bars;		/* enable/disable display of color bars */
	
	uint16				h_delay_adj;	/* adjust horizontal delay */
	uint16				v_delay_adj;	/* adjust vertical delay (should always be an even number) */

	uint32				version;		/* driver version		*/
	uint32				device_id;		/* device id			*/
	bool				pll;			/* pll enable/disable	*/
	uint32				stuffing[6];
} bt848_config;

typedef struct {						/* this struct is to be used with the BT848_ALLOCATE_BUFFER */
										/* and BT848_FREE_BUFFER driver ioctls */
	area_id				buffer_area;
	void				*buffer_address;
	uint32				buffer_size;
} bt848_buffer;

/* defines for video_source */
#define	BT848_COMPOSITE_2	0
#define	BT848_COMPOSITE		3
#define	BT848_TUNER			2
#define	BT848_SVIDEO		1
#define	BT848_COMPOSITE_ALT BT848_SVIDEO + 4
			
/* defines for video_format */
#define	BT848_NTSC_M		1
#define	BT848_NTSC_JAPAN	2
#define	BT848_PAL_BDGHI		3
#define BT848_PAL_M			4
#define	BT848_PAL_N			5
#define BT848_SECAM			6

/* defines for color_format */
								/* Pixel data with endian = LITTLE */
#define BT848_RGB32		0		/* DW0: 0  R  G  B  */
#define	BT848_RGB24		1		/* DW0: B1 R0 G0 B0 */
								/* DW1: G2 B2 R1 G1 */
								/* DW2: R3 G3 B3 R2 */
#define	BT848_RGB16		2		/* DW0: R1[7:3],G1[7:2],B1[7:3],R0[7:3],G0[7:2],B0[7:3]     */
#define BT848_RGB15		3		/* DW0: 0,R1[7:3],G1[7:3],B1[7:3],0,R0[7:3],G0[7:3],B0[7:3] */
#define BT848_YUV422	4		/* DW0: Cr0 Y1  Cb0 Y0  */
								/* DW1: Cr2 Y3  Bb2 Y2  */
#define BT848_YUV411	5		/* DW0: Y1  Cr0 Y0  Cb0 */
								/* DW1: Y3  Cr4 Y2  Cb4 */
								/* DW2: Y7  Y6  Y5  Y4  */
#define BT848_Y8		6		/* DW0: Y3  Y2  Y1  Y0  */
#define	BT848_RGB8		7		/* DW0: D3  D2  D1  D0  */

/* defines for capture mode */
#define BT848_CONTINUOUS	1
#define BT848_SINGLE		2

/* defines for address_type */
#define	BT848_LOGICAL		1
#define BT848_PHYSICAL		2

/* defines for endian */
#define	BT848_LITTLE		0
#define	BT848_BIG			1

/* defines for status */
#define BT848_VIDEO_PRESENT		0x80000000	/* 1 = video present				*/
#define BT848_HLOCK				0x40000000	/* 1 = horizontal lock				*/
#define BT848_NUM_LINES			0x10000000	/* 0 = 525  1 = 625  				*/
#define	BT848_RACK				0x02000000	/* 1 = successful I2C completion	*/ 
#define BT848_RISC_IRQ			0x00000800	/* 1 = Risc IRQ set, ie capture complete */
#define	BT848_I2CDONE			0x00000100	/* 1 = I2C complete					*/

/* defines for capture_status */
#define BT848_CAPTURE_OK			0
#define	BT848_CAPTURE_TIMEOUT		0x80000000 /* low order 28 bits of bt848 status reg  are ORed with this value */

/* defines for I2C status */
#define BT848_I2C_SUCCESS		 0
#define BT848_I2C_DONE_ERROR	-1
#define BT848_I2C_RACK_ERROR	-2

#endif  /* _Bt848_H */

