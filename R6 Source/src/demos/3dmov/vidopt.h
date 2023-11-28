#define HORIZONTAL_SIZE	160

#define VERTICAL_SIZE	120

#define BYTES_PER_PIXEL	4

#define BUFFER_SIZE			(HORIZONTAL_SIZE * VERTICAL_SIZE * BYTES_PER_PIXEL)
#define BUFFER_SIZE8K		(((HORIZONTAL_SIZE * VERTICAL_SIZE * BYTES_PER_PIXEL) + 8192)/8192)*8192

/* if FIELDS is not defined,the program captures only the odd fields (30 fields/sec) */
/* if FIELDS is defined, the program captures both odd and even fields (60 fields/sec) */
/* results will vary with your machine speed and picture size */
/* on a BeBox 66 you can capture 60 fields at QCIF (160 x 120), 30 fields at CIF (320 x 240 */
//#define FIELDS

