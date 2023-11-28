#ifndef SIS5598DEFS_H
#define SIS5598DEFS_H

/* SiS5598 chipset ID */
#define SIS_VENDORID 0x1039
#define SIS5598_DEVICEID 0x0200

#define	SIS5598_CLOCK_MAX		110000.0 // 135 MHz referring to SiS document, but if memory is at 55 MHz (default mode), 110 MHz dot_clock is more likely...

/* General Registers */
#define VGA_ENABLE 0x03c3
#define MISC_OUT_W 0x03c2
#define MISC_OUT_R 0x03cc
#define INPUT_STATUS_1 0x03da

/* Graphics Controller Registers */
#define GCR_INDEX  0x03ce
#define GCR_DATA   0x03cf

/* Attribute Controller and Video DAC Registers */
#define ATTR_REG   0x03c0

/* Color Registers */
#define DAC_RD_INDEX 0x03c7
#define DAC_WR_INDEX 0x03c8
#define DAC_DATA     0x03c9
#define DAC_ADR_MASK 0x03c6 

/* Sequencer Registers */
#define SEQ_INDEX  0x03c4
#define SEQ_DATA   0x03c5
#define SEQ_CLK_MODE 0x01

/* CRT Controller Registers */
#define CRTC_INDEX 0x03d4
#define CRTC_DATA  0x03d5
//#define CRTC_INDEX 0x03b4 text-mode registers
//#define CRTC_DATA 0x03b5

#endif
