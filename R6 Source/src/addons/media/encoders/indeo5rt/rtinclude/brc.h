/************************************************************************
*                                                                       *
*               INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                       *
*    This listing is supplied under the terms of a license agreement    *
*      with INTEL Corporation and may not be copied nor disclosed       *
*        except in accordance with the terms of that agreement.         *
*                                                                       *
*************************************************************************
*                                                                       *
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/
#if !defined __BRC_H__
#define __BRC_H__

/*
 * Contains the RTE bit rate control functions.
 */

#define BRC_ADJUST_PERIOD	1		/* how often (in # of frames) GlobalQuant is
									 * adjusted.
									 */
#define BRC_ADJUST_MARGIN	5		/* 100 divided by this number is the margin (in %)
									 * within which no adjust is made when comparing
                                                                         * actual with requested.
									 */
#define BRC_MINKEYADJUST	6		/* for global quant levels equal to or greater than
									 * this value, decrease the quant level for key frames.
									 */
#define BRC_KEYADJUST       2		/* how much to decrease quant level for key frames */


#define BRC_AVG_PERIOD		15		/* number of frames over which data rate average 
									 * is calculated.
									 */
#endif
