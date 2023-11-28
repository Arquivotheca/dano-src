/****************************************************************************
 All software on this website is made available under the following terms and
 conditions.  By downloading this software, you agree to abide by these terms
 and conditions with respect to this software.

 I2O SIG All rights reserved.

 These header files are provided, pursuant to your I2O SIG membership agreement,
 free of charge on an as-is basis without warranty of any kind, either express
 or implied, including but not limited to, implied warranties or merchantability
 and fitness for a particular purpose.  I2O SIG does not warrant that this
 program will meet the user's requirements or that the operation of these
 programs will be uninterrupted or error-free. Acceptance and use of this
 program constitutes the user's understanding that he will have no recourse
 to I2O SIG for any actual or consequential damages including, but not limited
 to, loss profits arising out of use or inability to use this program.

 Member is permitted to create derivative works to this header-file program.
 However, all copies of the program and its derivative works must contain
 the I2O SIG copyright notice.
**************************************************************************/

#ifndef __INCi2omoduleh
#define __INCi2omoduleh

#define I2OMODULE_REV 1_5_5

#include "i2otypes.h"

/* major capabilities bit definitions */

#define I2O_MODULE_32_BIT_CONTEXT_SUPPORT          0x0
#define I2O_MODULE_64_BIT_CONTEXT_SUPPORT          0x1
#define I2O_MODULE_32_OR_64_BIT_CONTEXT_SUPPORT    0x2
#define I2O_MODULE_32_AND_64_BIT_CONTEXT_SUPPORT   0x3


/* module header table types */

#define I2O_MODULE_INDEX_TABLE         0x0000
#define I2O_MODULE_ADAPTER_TABLE       0x0001
#define I2O_MODULE_DEVICE_TABLE        0x0002
#define I2O_MODULE_OBSOLETE_DDM_TABLE  0x0003
#define I2O_MODULE_TCL_TABLE           0x0004


/* Module header */

typedef struct
    {
    U32         headerSize;         /* size of this header and tables */
    U16         orgId;              /* I2O organization ID */
    U16         moduleId;           /* assigned to vendor of module */
    U16         day;                /* ascii 4 digit day DDM produced */
    U16         month;              /* ascii 4 digit month DDM produced */
    U32         year;               /* ascii 4 digit year DDM produced */
    U8          i2oVersion;         /* I2O version info */
    U8          majorCapabilities;  /* capbilities bits */
    U16         reserved;           /* reserved */
    U32         codeSize;           /* text/data/bss */
    U32         tableOffset;        /* offset to numTables */
    U32         memoryReq;          /* pre-attach memory requiremets */
    U32         memoryPreferred;    /* additional desired */
    char        moduleVersion[4];   /* 4 ascii characters */
    U8          processorType;      /* IOP processor type */
    U8          processVersion;     /* IOP processor type */
    U8          objCodeFormat;      /* DDM object module format */
    U8          reserved1;          /* reserved */
    U32         numTables;          /* number of descriptor tables */
    char        moduleInfo[24];     /* ascii string name */
    } I2O_MODULE_DESC_HDR;


/* Module Parameter Block */

typedef struct
    {
    U32             mpbSize;        /* size of this header and tables */
    U16             orgId;          /* I2O organization ID */
    U16             modId;          /* assigned to vendor of module */
    U32             mpbVersion;     /* MPB version info */
    U32             reserved;       /* reserved */
    } I2O_MODULE_PARAM_BLK;

/* Generic header for module tables. This applies to all tables in the
 * module header apart from the TCL script table, which does not define
 * the entrySize and numEntries fields.
 */

typedef struct
    {
    U16      length;        /* length of table in 32 bit words */
    U16      descriptorId;  /* descriptor ID = 0x0000 */
    U8       entrySize;     /* size of entries in 32 bit words */
    U8       numEntries;    /* number of entries */
    U8       tableVersion;  /* table version */
    U8       reserved;      /* reserved */
} I2O_MODULE_TABLE;

typedef struct
    {
    U16      tableDescId;   /* descriptor ID of table */
    U16      reserved;      /* reserved */
    U32      tableOffset;   /* offset in bytes of table from start
of mod hdr */
} I2O_INDEX_TABLE_ENTRY;

typedef struct
    {
    U32      classId;       /* Message class */
    U32      subClass;      /* Subclass */
} I2O_DEVICE_TABLE_ENTRY;

typedef struct
    {
    U16      orgId;         /* Organization ID */
    U16      modId;         /* Module ID */
} I2O_OBSOLETE_DDM_TABLE_ENTRY;

#endif /* __INCi2omoduleh */
