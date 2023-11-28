/* ++++++++++
	$Source: /net/bally/be/rcs/src/kernel/joe/cs4231.h,v $
	$Revision: 1.3 $
	$Author: herold $
	$Date: 1995/06/08 21:01:22 $
	Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.

	Definitions for the Crystal CS4231 sound chip.
+++++ */

#ifndef _CS4231_H
#define _CS4231_H

#define CS_base			0x830	/* I/O address of sound chip registers */

/* offsets to directly accessed registers */

#define CS_index		0		/* index to indirectly accessed register */
#define CS_data			1		/* data to/from indirectly accessed reg */
#define CS_status		2		/* codec status */
#define CS_pio			3		/* playback/capture data, in pio mode */


/* offset to registers accessed indirectly thru the index/data registers */

#define CS_left_adc_control			0	/* left ADC input control */
#define CS_right_adc_control		1	/* right ADC input control */
#define CS_left_aux1_in_control		2	/* left aux #1 input control */
#define CS_right_aux1_in_control	3	/* right aux #1 input control */
#define CS_left_aux2_in_control		4	/* left aux #2 input control */
#define CS_right_aux2_in_control	5	/* right aux #2 input control */
#define CS_left_dac_control			6	/* left ouput control */
#define CS_right_dac_control		7	/* right ouput control */
#define CS_clock_play_data_format	8	/* clock & playback data format */
#define CS_interface_config			9	/* interface configuration */
#define CS_pin_control				10	/* pin control */
#define CS_error_init				11	/* error status and initialization */
#define CS_mode_id					12	/* mode and id */
#define CS_loopback_control			13	/* loopback control */
#define CS_play_upper_base			14	/* playback upper base count */
#define CS_play_lower_base			15	/* playback lower base count */
#define CS_alt_feature_enable_1		16	/* alternate feature enable 1 */
#define CS_alt_feature_enable_2		17	/* alternate feature enable 2 */
#define CS_left_line_in_control		18	/* left line input control */
#define CS_right_line_in_control	19	/* right line input control */
#define CS_timer_low				20	/* timer low byte */
#define CS_timer_high				21	/* timer high byte */
#define CS_alt_feature_status		24	/* alternate feature status */
#define CS_version_id				25	/* version/chip id */
#define CS_mono_in_out_control		26	/* mono input and output control */
#define CS_capture_data_format		28	/* capture data format */
#define CS_capture_upper_base		30	/* capture upper base count */
#define CS_capture_lower_base		31	/* capture lower base count */

/* bit definitions in the index register */

#define CS_index_addr		0x0F	/* index to indectly accesed register */
#define CS_index_trd		0x20	/* transfer request disable */
#define CS_index_mce		0x40	/* mode change enable */
#define CS_index_init		0x80	/* (r/o) initialization underway */

/* bit definitions for the status register */

#define CS_status_int		0x01	/* interrupt pin state */
#define CS_status_prdy		0x02	/* (r/o) playback data register ready */
#define CS_status_plr		0x04	/* (r/o) 1/0 left/right playback sample */
#define CS_status_pul		0x08	/* (r/o) 1/0 upper/lower playback byte */
#define CS_status_sour		0x10	/* (r/o) sample over-or-underrun */
#define CS_status_crdy		0x20	/* (r/o) capture data register ready */
#define CS_status_clr		0x40	/* (r/o) 1/0 left/right capture sample */
#define CS_status_cul		0x80	/* (r/o) 1/0 upper/lower capture byte */

/* bit defintions for the left & right ADC input control registers */

#define CS_adc_gain			0x0F	/* gain, in step of 1.5 dB */
#define CS_adc_mge			0x20	/* microphone gain enable */
#define CS_adc_src_line		0x00	/* source = line */
#define CS_adc_src_aux1		0x40	/* source = aux1 */
#define CS_adc_src_mic		0x80	/* source = mic */
#define CS_adc_src_dac		0xC0	/* source = post-mixed DAC output */


/* bit definitions for left & right auxiliary # 1 & 2 input control */

#define CS_iaux_attn		0x1F	/* aux1 input attenuation: +12 DB -> -34.5 dB */
#define CS_iaux_mute		0x80	/* aux1 input mute */

/* bit definitions for left & right DAC control */

#define CS_dac_attn			0x3F	/* dac attenuation, in 1.5 dB steps */
#define CS_dac_mute			0x80	/* dac mute */


/* bit definitions for the clock & data format register. More useful
   definitions for specific sample rates and data formats follow. */

#define CS_format_css		0x01	/* clock source select */
#define CS_format_cfs		0x0E	/* clock frequency divide select */
#define CS_format_stereo	0x10	/* 1/0 stereo/mono */
#define CS_format_compand	0x20	/* 1/0 companded/linear PCM */
#define CS_format_fmt		0xC0	/* format select */

/* sample rate definitions for the clock & data format register */

#define CS_5_5125_kHz		0x01	/* 5.5125 kHz */
#define CS_6_615_kHz		0x0F	/* 6.615 kHz */
#define CS_8_0_kHz			0x00	/* 8.0 kHz */
#define CS_9_6_kHz			0x0E	/* 9.6 kHz */
#define CS_11_025_kHz		0x03	/* 11.025 kHz */
#define CS_16_0_kHz			0x02	/* 16.0 kHz */
#define CS_18_9_kHz			0x05	/* 18.9 kHz */
#define CS_22_05_kHz		0x07	/* 22.05 kHz */
#define CS_27_42857_kHz		0x04	/* 27.42857 kHz */
#define CS_32_0_kHz			0x06	/* 32.0 kHz */
#define CS_33_075_kHz		0x0D	/* 33.075 kHz */
#define CS_37_8_kHz			0x09	/* 37.8 kHz */
#define CS_44_1_kHz			0x0B	/* 44.1 kHz */
#define CS_48_0_kHz			0x0C	/* 48.0 kHz */

/* bit definitions for the interface configuration register */

#define CS_config_pen		0x01	/* playback enable */
#define CS_config_cen		0x02	/* capture enable */
#define CS_config_sdc		0x04	/* 1/0 single/dual dma channel */
#define CS_config_acal		0x08	/* autocalibrate enable */
#define CS_config_ppio		0x40	/* 1/0 pio/dma playback */
#define CS_config_cpio		0x80	/* 1/0 pio/dma capture */

/* bit definitions for the pin control register */

#define CS_pin_ien			0x02	/* interrupt enable */
#define CS_pin_den			0x08	/* 16->8 bit dither enable */
#define CS_pin_xctl0		0x40	/* external control pin 0 state */
#define CS_pin_xctl1		0x80	/* external control pin 1 state */

/* bit definitions for the error status and initialization register */

#define CS_estat_left_over	0x03	/* overrange left detect */
#define CS_estat_right_over	0x0C	/* overrange right detect */
#define CS_estat_drs		0x10	/* pdrq or cdrq status (play/capture dma req active) */
#define CS_estat_aci		0x20	/* autocalibrate in progress */
#define CS_estat_pur		0x40	/* playback underrun */
#define CS_estat_cor		0x80	/* capture overrun */

/* bit definitions for the mode and id register */

#define CS_mode_revision	0x0F	/* chip revision id */
#define CS_mode_mode2		0x40	/* enable expanded mode */

/* bit definitions for the loopback control register */

#define CS_loop_dme			0x01	/* loopback enable of DAC output to ADC input */
#define CS_loop_attn		0xFC	/* attenuation of mix ADC data, in -1.5 dB steps */


/* bit definitions for the alternate feature enable register 1 */

#define CS_alt1_dacz		0x01	/* 1/0 DAC center scale/last sample on underrun */
#define CS_alt1_te			0x40	/* timer enable */
#define CS_alt1_olb			0x80	/* 1/0 analog output 2.8/2.0 full scale */


/* bit definitions for the alternate feature enable register 2 */

#define CS_alt2_hpf			0x01	/* 1/0 en/disable high pass ADC filter */


/* bit definitions for the left & right line input control registers */

#define CS_line_in_mix_gain	0x1F	/* gain of line input to mixer, 12 to -34.5 dB in -1.5 dB steps */
#define CS_line_in_mix_mute	0x80	/* mute line input to mixer */


/* bit definitions for the alternate feature status register */

#define CS_alt_status_pu	0x01	/* playback underrun occurred */
#define CS_alt_status_po	0x02	/* playback overrun occurred */
#define CS_alt_status_co	0x04	/* capture overrun occurred */
#define CS_alt_status_cu	0x08	/* capture underrun occurred */
#define CS_alt_status_pi	0x10	/* playback DMA count interrupt pending */
#define CS_alt_status_ci	0x20	/* capture DMA count interrupt pending */
#define CS_alt_status_ti	0x40	/* timer interrupt pending */


/* bit definitions for the version/id register */

#define CS_id_cid			0x07	/* chip id */
#define CS_id_version		0xE0	/* version number */


/* bit definitions for the mono input and output control register */

#define CS_mono_mia			0x0F	/* mono input attenuation, in 3 dB steps */
#define CS_mono_mom			0x40	/* mono output mute */
#define CS_mono_mim			0x80	/* mono input mute */


/* data format definitions for the clock & data format register, as well
   as the capture data format register */

#define CS_8_pcm_mono		0x00	/* 8 bit unsigned pcm, mono */
#define CS_8_pcm_stereo		0x10	/* 8 bit unsigned pcm, stereo */
#define CS_8_ulaw_mono		0x20	/* 8 bit u-law companded, mono */
#define CS_8_ulaw_stereo	0x30	/* 8 bit u-law companded, stereo */
#define CS_16_lpcm_mono		0x40	/* 16 bit twos-complement little endian pcm, mono */
#define CS_16_lpcm_stereo	0x50	/* 16 bit twos-complement little endian pcm, stereo */
#define CS_8_alaw_mono		0x60	/* 8 bit a-law companded, mono */
#define CS_8_alaw_stereo	0x70	/* 8 bit a-law companded, stereo */
#define CS_4_adpcm_mono		0xA0	/* 4 bit ADPCM, mono */
#define CS_4_adpcm_stereo	0xB0	/* 4 bit ADPCM, stereo */
#define CS_16_pcm_mono		0xC0	/* 16 bit twos complement big endian pcm, mono */
#define CS_16_pcm_stereo	0xD0	/* 16 bit twos complement big endian pcm, stereo */


#endif
