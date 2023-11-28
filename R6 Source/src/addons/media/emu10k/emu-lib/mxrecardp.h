/*****************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1998. All rights Reserved.
*					
******************************************************************************
* @doc INTERNAL
* @module mxrecardp.h | 
*  This module contains private but usable definitions for applications
*  which have detailed knowledge of the inner workings of the ECARD mixer
*  implementation.  So far, the only expected client of this stuff is 
*  the manufacturing build and test code.  No one else has any business
*  calling these routines, and I will personally hunt you down and kill
*  you if you call them and screw up the EEPROM.
*
* @iex
* Revision History:
* Version	Person		Date		Reason
* -------   ---------	----------	--------------------------------------
*  0.001	JK			Feb 5, 1998 Contains private but exported stuff.
* @end
******************************************************************************
*/

#ifndef _MXRECARDP_H
#define _MXRECARDP_H

BEGINEMUCTYPE
/* Miscellaneous PROM stuff */
#define EC_CURRENT_PROM_VERSION 0x01 /* Self-explanatory.  This should
                                      * be incremented any time the EEPROM's
                                      * format is changed.  */

#define EC_EEPROM_SIZE	        0x40 /* ECARD EEPROM has 64 16-bit words */

/* Addresses for special values stored in to EEPROM */
#define EC_PROM_VERSION_ADDR 0x20    /* Address of the current prom version */
#define EC_BOARDREV0_ADDR    0x21    /* LSW of board rev */
#define EC_BOARDREV1_ADDR    0x22    /* MSW of board rev */ 

#define EC_LAST_PROMFILE_ADDR 0x2f

#define EC_SERIALNUM_ADDR    0x30    /* First word of serial number.  The 
                                      * can be up to 30 characters in length
                                      * and is stored as a NULL-terminated
                                      * ASCII string.  Any unused bytes must be
                                      * filled with zeros */
#define EC_CHECKSUM_ADDR     0x3f    /* Location at which checksum is stored */

/* Semi-private exported functions.  */
extern void _ECARDwrite_eeprom(HALID, WORD, WORD);
extern WORD _ECARDread_eeprom(HALID, WORD);
extern void _ECARDread_serialnum(HALID, WORD, CHAR *);
extern void _ECARDwrite_serialnum(HALID, WORD, CHAR *);
extern WORD _ECARDcompute_checksum(HALID);

ENDEMUCTYPE

#endif /* _MXRECARDP_H */
