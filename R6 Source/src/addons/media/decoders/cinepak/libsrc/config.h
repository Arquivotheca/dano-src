/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/config.h 2.3 1994/05/09 17:25:57 bog Exp $

 * (C) Copyright 1992-1993 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: config.h $
 * Revision 2.3  1994/05/09 17:25:57  bog
 * Black & White compression works.
 * Revision 2.2  1993/06/17  11:38:13  geoffs
 * Fix up dialog box.
 * 
 * Revision 2.1  93/06/16  15:55:54  geoffs
 * Remove unused dialog box definition, minor update to version info stuff
 * 
 * Revision 2.0  93/06/01  14:13:19  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.3  93/04/21  15:46:43  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.2  92/11/11  13:00:13  geoffs
 * We now use the version resource to display info in dialog boxes
 * 
 * Revision 1.1  92/10/28  13:27:02  geoffs
 * Initial revision
 * 
 */

#define ID_PRODUCTNAME              120
#define ID_PRODUCTVERSION           121
#define ID_COPYRIGHT                122

#ifndef NOBLACKWHITE

#define ID_COLOR                    101
#define ID_BLACKANDWHITE            102

#endif
