
/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*
* File: e10k1reg.h
*
* This file contains contains register definitions for the E10K1 chip.
* It is included by hal8210.h.
*/

#ifndef _E10K1REG_H
#define _E10K1REG_H


/************************ Function 0 Register Map ********************
*
* The following are the offsets within the Function 0 I/O space at which
* 8010 registers are mapped. Function 0 handles the sound engine.
*
*/ 

/* Sound Engine pointer and data registers */
#define PTRREG  0x0000U 
#define DATAREG 0x0004U 

/* Interrrupt pending and Interrupt enable reigster */
#define INTP   0x0008U
#define INTE   0x000CU

/* Wall clock */
#define WC     0x0010U

/* "Hardware control"  register and bit values*/
#define HC     0x0014U
#define  HC_JSTK      0x200L   /* Enable joystick */
#define  HC_AM         0x10L   /* Auto mute */
#define  HC_LC         0x08L   /* Lock sound engine cache */
#define  HC_LT         0x04L   /* Lock fx tank ram */
#define  HC_MD         0x02L   /* Mute Disables Audio */
#define  HC_EA         0x01L   /* Enable audio */
/* Codec types - warning! Wrong type will look like a system hang. */
#define	 HC_CF		   0x00070000L
#define  HC_CF_AC97	   0x00000000L
#define  HC_CF_I2S	   0x00010000L
/* Gosh, there are more, but we aren't using them */
	   

/* Midi UART Data and Command/Status registers - byte-wide */
#define MUDTA  0x0018U
#define MUCMD  0x0019U
#define MUSTA  0x0019U
#define	 MUSTA_IRDYN	0x80
#define	 MUSTA_ORDYN	0x40

/* Timer interrupt register - holds the interrupt period in samples */
#define TIMR   0x001AU

/* AC97 I/F registers - Data and Address/Status */
#define AC97D  0x001CU
#define AC97A  0x001EU


/************************ Function 1 Register Map ********************
*
* The following are the offsets within the Function 1 I/O space at which
* 8010 registers are mapped. Function 1 handles the joystick interface.
*
*/ 

#define JOYRD	0x0000U
#define JOYWD	0x0000U


/**************** Sound Engine Channel Helpers   ********************
*
* These constants are register offsets that are programmed into
* the Function 0 pointer register PTRREG so that the 8010's internal 
* register space can be accessed through the data register DATAREG.
* Most are for controlling sound engine channels and are duplicated for
* each of the 64 channels.  These constants are the "RGA" addresses shifted 
* left by 16 so that they can be OR'd with the channel number in the LS 6 bits
* to get the specific value to program into the pointer register.
* When the driver reads or writes the 8010 it will OR the  channel number  
* with the RGA and then do a 32 bit OUT to the PTRREG to set up the 
* address, followed by a 32-bit IN or OUT to the DATAREG to read or write the
* data, ie: 
*
*          _outd( FunctionIOAddress+PTRREG, PTAB | channelNum);  
*/

#define EMU8010NUMCHANS 64

/* Channel registers */
#define CPF    0x00000000   /* Current Pitch and Fraction */
#define PTAB   0x00010000   /* Pitch Target and Sends A and B */
#define CVCF   0x00020000   /* Current Volume, Filter Cutoff */
#define VTFT   0x00030000   /* Volume Target, Filter cutoff Target */
#define Z2     0x00040000   /* delay memory */
#define Z1     0x00050000   /* delay memory */
#define SCSA   0x00060000   /* send C and start address */
#define SDL    0x00070000   /* send D and loop address */
#define QKBCA  0x00080000   /* Filter Q, ROM, Byte size, current addr */
#define FXRT   0x000b0000   /* FX routing */
#define CCR    0x00090000   /* cache control */
#define CLP    0x000a0000   /* cache loop */
#define MAPA   0x000c0000   /* cache map A */
#define MAPB   0x000d0000   /* cache map B */

/* Envelope Registers */
#define VEV     0x00100000   /* Volume Envelope  Value */
#define VEHA    0x00110000   /* Volume Envelope Hold and Attack   */
#define VEDS    0x00120000   /* Volume Envelope Decay and Sustain */
#define MLV     0x00130000   /* Modulation LFO Value */
#define MEV     0x00140000   /* Modulation Envelope Value */
#define MEHA    0x00150000   /* Modulation Hold and Attack */
#define MEDS    0x00160000   /* Modulation Decay and Sustain */
#define VLV     0x00170000   /* Volume LFO value */
#define IP      0x00180000   /* Initial Pitch*/
#define IFA     0x00190000   /* Initial Filter cutoff and Attenuation */
#define PEFE    0x001a0000   /* Pitch Envelope and Filter Enveolpe amount */
#define VFM     0x001b0000   /* Vibrato and Filter Modulation from Mod LFO */
#define TMFQ    0x001c0000   /* Tremolo and Modulation LFO Frequency*/
#define VVFQ    0x001d0000   /* Vibrato and Vibrato LFO Frequency*/
#define TMPE    0x001e0000   /* Tempory Envelope Register */

/* Data regs for the channel's cache data, 0 to 0xF in 32-bit words */
#define CD0     0x00200000    
#define CD1     0x00210000    
#define CD2     0x00220000    
#define CD3     0x00230000    
#define CD4     0x00240000    
#define CD5     0x00250000    
#define CD6     0x00260000    
#define CD7     0x00270000    
#define CD8     0x00280000    
#define CD9     0x00290000    
#define CDA     0x002A0000    
#define CDB     0x002B0000    
#define CDC     0x002C0000    
#define CDD     0x002D0000    
#define CDE     0x002E0000    
#define CDF     0x002F0000    

/* All the following global registers are mapped into sound engine "channel"s, 
* but there are not actually individual per-channel versions of each
* register - there are identical "aliases" of global registers that are
* found within each channel.
* Thus when accessing these registers the channel offset is irrelevant.
*/ 
#define PTBA    0x00400000   /* page Table Base address  */
#define TCBA    0x00410000   /* tank cache Base address  */
#define ADCSR   0x00420000   /* ADC sample rate and control */
#define FXWC    0x00430000   /* FX engine output write channels */
#define TCBS    0x00440000   /* tank cache buffer size  */ 
#define MBA     0x00450000   /* Microphone Buffer Address */
#define ADCBA   0x00460000   /* ADC buffer Address */
#define FXBA    0x00470000   /* FX engine output buffer Address */
#define GPSBS   0x00480000   /* General purpose S/PDIF buffer size */
#define MBS     0x00490000   /* Microphoine buffer size */
#define ADCBS   0x004a0000   /* ADC buffer size */
#define FXBS    0x004b0000   /* FX buffer size */
#define CDCS    0x00500000   /* CD ROM channel status */
#define GPSCS   0x00510000   /* General purpose S/PDIF channel status */

#define DBG     0x00520000   /* FX engine debug */
#define FX_UNUSED 0x00530000  
#define SCS0    0x00540000   /* S/PDIF output 0 Channel Status */
#define SCS1    0x00550000   /* S/PDIF output 1 Channel Status */
#define SCS2    0x00560000   /* S/PDIF output 2 Channel Status */

#define CLIEL   0x00580000   /* Channel Loop interrupt enable (low)  */
#define CLIEH   0x00590000   /* Channel Loop interrupt enable (high)  */
#define CLIPL   0x005a0000   /* Channel Loop interrupt pending (low)  */
#define CLIPH   0x005b0000   /* Channel Loop interrupt pending (high)  */
#define SOLL    0x005c0000   /* Channel stop on loop (low)  */
#define SOLH    0x005d0000   /* Channel stop on loop (high)  */

#define CDSRCS  0x00600000   /* CD Sample Rate Converter Status */
#define GPSRCS  0x00610000   /* GP S/PDIF Sample Rate Converter Status */
#define ZVSRCS  0x00620000   /* Zvideo Sample Rate Converter Status */
#define MIDX    0x00630000   /* Microphone buffer index */
#define ADCIDX  0x00640000   /* ADC Buffer Index */
#define FXIDX   0x00650000   /* FX Output buffer index */

/* Interrupt enable bit fields */
#define EINTE_SRT 0x2000
#define EINTE_FX  0x1000 
/* Note: No enable bit for Host Modem! */
#define EINTE_PCI 0x0800
#define EINTE_VI  0x0400
#define EINTE_VD  0x0200
#define EINTE_MU  0x0100
#define EINTE_MB  0x0080
#define EINTE_AB  0x0040
#define EINTE_FB  0x0020
#define EINTE_SS  0x0010
#define EINTE_CS  0x0008
#define EINTE_IT  0x0004
#define EINTE_TX  0x0002
#define EINTE_RX  0x0001

/* Interrupt pending bit positions */
#define EINT_SRT 24
#define EINT_FX  23
#define EINT_HM  22
#define EINT_PCI 21
#define EINT_VI  20
#define EINT_VD  19
#define EINT_MU  18
#define EINT_MF  17
#define EINT_MH  16
#define EINT_AF  15
#define EINT_AH  14
#define EINT_FF  13
#define EINT_FH  12
#define EINT_SS  11
#define EINT_CS  10
#define EINT_IT   9
#define EINT_TX   8
#define EINT_RX   7
#define EINT_CL   6

/* Interrupt pending bit fields */
#define EINTP_SRT 0x01000000
#define EINTP_FX  0x00800000
#define EINTP_HM  0x00400000
#define EINTP_PCI 0x00200000
#define EINTP_VI  0x00100000
#define EINTP_VD  0x00080000
#define EINTP_MU  0x00040000
#define EINTP_MF  0x00020000
#define EINTP_MH  0x00010000
#define EINTP_AF  0x00008000
#define EINTP_AH  0x00004000
#define EINTP_FF  0x00002000
#define EINTP_FH  0x00001000
#define EINTP_SS  0x00000800
#define EINTP_CS  0x00000400
#define EINTP_IT  0x00000200
#define EINTP_TX  0x00000100
#define EINTP_RX  0x00000080
#define EINTP_CL  0x00000040

#endif /* _E10K1REG_H */
