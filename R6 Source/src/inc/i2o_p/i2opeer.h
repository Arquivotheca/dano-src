/**********************************************************************
 * All software on this website is made available under the following
 * terms and conditions.  By downloading this software, you agree to
 * abide by these terms and conditions with respect to this software.
 *
 * I2O SIG All rights reserved.
 *
 * These header files are provided, pursuant to your I2O SIG membership
 * agreement, free of charge on an as-is basis without warranty of any
 * kind, either express or implied, including but not limited to,
 * implied warranties or merchantability and fitness for a particular
 * purpose.  I2O SIG does not warrant that this program will meet the
 * user's requirements or that the operation of these programs will be
 * uninterrupted or error-free.  Acceptance and use of this program
 * constitutes the user's understanding that he will have no recourse
 * to I2O SIG for any actual or consequential damages including, but
 * not limited to, loss profits arising out of use or inability to use
 * this program.
 *
 * Member is permitted to create deriavative works to this header-file
 * program.  However, all copies of the program and its derivative
 * works must contain the I2O SIG copyright notice.
 *********************************************************************/

/*********************************************************************
 * I2OPeer.h -- I2O Peer definition file
 *
 * This file contains information presented in Addendum B to
 * the I2O(tm) Specification - Peer-to-Peer operation
 * 
 **********************************************************************/

#if !defined(I2O_PEER_HDR)
#define I2O_PEER_HDR

#define I2OPEER_REV 1_5_5  /* I2OPeer header file revision string */


/* Peer Transport Agent Parameter Group Numbers */

#define    I2O_PTA_ACCESS_GROUP_NO                     0x0000
#define    I2O_PTA_IOP_CONNECT_TABLE_GROUP_NO          0x0001
#define    I2O_PTA_SERVICE_REGISTRATION_GROUP_NO       0x0002
#define    I2O_PTA_PSU_SEND_TABLE_GROUP_NO             0x0003
#define    I2O_PTA_PSU_MOVE_TABLE_GROUP_NO             0x0004
#define    I2O_PTA_PSU_REDIR_TABLE_GROUP_NO            0x0005
#define    I2O_PTA_PSU_REMOTE_MEMORY_TABLE_GROUP_NO    0x0006
#define    I2O_PTA_PSU_RECEIVE_TABLE_GROUP_NO          0x0007

/* - 0000h - Access Group */

#define    I2O_PTA_ACCESS_I2O_VENDOR_ID                0x0000
#define    I2O_PTA_ACCESS_SDU_VERSION                  0x0001
#define    I2O_PTA_ACCESS_NUM_BOUND_TRANSPORT          0x0002
#define    I2O_PTA_ACCESS_NUM_CONNECTED_IOPS           0x0003
#define    I2O_PTA_ACCESS_NUM_IOP_PATHS                0x0004
#define    I2O_PTA_ACCESS_NUM_ACTIVE_LINKS             0x0005
#define    I2O_PTA_ACCESS_MAX_REDIR_MEM_ALLOC          0x0006
#define    I2O_PTA_ACCESS_MAX_REM_MEM_ALLOC            0x0007
#define    I2O_PTA_ACCESS_SERVICE_FLAGS                0x0008
#define    I2O_PTA_ACCESS_CURR_REDIR_MEM_ALLOC         0x0009
#define    I2O_PTA_ACCESS_CURR_REM_MEM_ALLOC           0x000A

/* Memory Allocation Values */

#define    I2O_PTA_ACCESS_NO_MEM_LIMIT                0xFFFFFFFF

/* Service Flags Definitions */

#define    I2O_PTA_ACCESS_SVCFLAGS_FAILOVER_BIT       0x00000001

#define    I2O_PTA_SVCFLAGS_FAILOVER_NOTSUPPORTED     0x00000000
#define    I2O_PTA_SVCFLAGS_FAILOVER_SUPPORTED        0x00000001

typedef struct _I2O_PTA_ACCESS_SCALAR {
    U16    I2oVendorId;
    U16    PtaSduVersion;
    U32    NumBoundTransport;
    U32    NumConnectedIops;
    U32    NumIopPaths;
    U32    NumActiveLinks;
    U32    MaxRedirMemAlloc;
    U32    MaxRemMemAlloc;
    U32    PtaServiceFlags;
    U32    CurrRedirMemAlloc;
    U32    CurrRemMemAlloc;
} I2O_PTA_ACCESS_SCALAR, *PI2O_PTA_ACCESS_SCALAR;

/* - 0001h - IOP Connect Group */

#define    I2O_PTA_IOP_CONNECT_CONNECTINDEX           0x0000
#define    I2O_PTA_IOP_CONNECT_PTTID                  0x0001
#define    I2O_PTA_IOP_CONNECT_CONNECTUNIT            0x0002
#define    I2O_PTA_IOP_CONNECT_CONNECTIOP             0x0003
#define    I2O_PTA_IOP_CONNECT_CONNECTSTATE           0x0004
#define    I2O_PTA_IOP_CONNECT_MAXTRANSDATA           0x0005

/* IOP Connect State Values */

#define    I2O_PTA_IOP_CONNECT_STATE_ACTIVE           0x0000
#define    I2O_PTA_IOP_CONNECT_STATE_FAULTED          0x0001
#define    I2O_PTA_IOP_CONNECT_STATE_NONACTIVE        0x0002
#define    I2O_PTA_IOP_CONNECT_STATE_STANDBY          0x0003

typedef struct _I2O_PTA_IOP_CONNECT_TABLE {
    U32    ConnectIndex;
    U16    PtId;
    U16    ConnectUnit;
    U16    ConnectIop;
    U16    ConnectState;
    U32    MaxTransData;
} I2O_PTA_IOP_CONNECT_TABLE, *PI2O_PTA_IOP_CONNECT_TABLE;

/* - 0002h - PSU Service Registration Group */

#define    I2O_PTA_SERVICE_NUM_SEND_OBJ               0x0000
#define    I2O_PTA_SERVICE_NUM_MOVE_OBJ               0x0001
#define    I2O_PTA_SERVICE_NUM_REDIR_OBJ              0x0002
#define    I2O_PTA_SERVICE_NUM_REM_MEM_OBJ            0x0003
#define    I2O_PTA_SERVICE_NUM_REC_OBJ                0x0004

typedef struct _I2O_PTA_SERVICE_REG_SCALAR {
    U32    NumSendObj;
    U32    NumMoveObj;
    U32    NumRedirObj;
    U32    NumRemMemObj;
    U32    NumRecObj;
} I2O_PTA_SERVICE_REG_SCALAR, *PI2O_PTA_SERVICE_REG_SCALAR;

/* - 0003h - PSU Send Service Group */

#define    I2O_PTA_SEND_SERVICE_PSU_INDEX                  0x0000
#define    I2O_PTA_SEND_SERVICE_PSU_TID                    0x0001
#define    I2O_PTA_SEND_SERVICE_PT_TID                     0x0002
#define    I2O_PTA_SEND_SERVICE_REMOTE_UNIT                0x0003
#define    I2O_PTA_SEND_SERVICE_REMOTE_IOP                 0x0004
#define    I2O_PTA_SEND_SERVICE_REMOTE_TID                 0x0005

typedef struct _I2O_PTA_SEND_SERVICE_TABLE {
    U32    PsuIndex;
    U16    PsuTid;
    U16    PtTid;
    U16    RemUnit;
    U16    RemIop;
    U32    RemTid;
} I2O_PTA_SEND_SERVICE_TABLE, *PI2O_PTA_SEND_SERVICE_TABLE;

/* - 0004h - PSU Move Service Group */

#define    I2O_PTA_MOVE_SERVICE_PSU_INDEX                  0x0000
#define    I2O_PTA_MOVE_SERVICE_PSU_TID                    0x0001
#define    I2O_PTA_MOVE_SERVICE_PT_TID                     0x0002
#define    I2O_PTA_MOVE_SERVICE_REMOTE_UNIT                0x0003
#define    I2O_PTA_MOVE_SERVICE_REMOTE_IOP                 0x0004

typedef struct _I2O_PTA_MOVE_SERVICE_TABLE {
    U32    PsuIndex;
    U16    PsuTid;
    U16    PtTid;
    U16    RemUnit;
    U16    RemIop;
} I2O_PTA_MOVE_SERVICE_TABLE, *PI2O_PTA_MOVE_SERVICE_TABLE;

/* - 0005h - PSU Redirect Service Group */

#define    I2O_PTA_REDIR_SERVICE_PSU_INDEX                  0x0000
#define    I2O_PTA_REDIR_SERVICE_PSU_TID                    0x0001
#define    I2O_PTA_REDIR_SERVICE_PT_TID                     0x0002
#define    I2O_PTA_REDIR_SERVICE_REMOTE_UNIT                0x0003
#define    I2O_PTA_REDIR_SERVICE_REMOTE_IOP                 0x0004
#define    I2O_PTA_REDIR_SERVICE_REMOTE_TID                 0x0005
#define    I2O_PTA_REDIR_SERVICE_RESERVED1                  0x0006
#define    I2O_PTA_REDIR_SERVICE_REDIR_SIZE                 0x0007

typedef struct _I2O_PTA_REDIR_SERVICE_TABLE {
    U32    PsuIndex;
    U16    PsuTid;
    U16    PtTid;
    U16    RemUnit;
    U16    RemIop;
    U16    RemTid;
    U8     Reserved[2];
    U32    RedirecSize;
} I2O_PTA_REDIR_SERVICE_TABLE, *PI2O_PTA_REDIR_SERVICE_TABLE;

/* - 0006h - PSU Remote Allocate Service Group */

#define    I2O_PTA_REMALLOC_SERVICE_PSU_INDEX               0x0000
#define    I2O_PTA_REMALLOC_SERVICE_PSU_TID                 0x0001
#define    I2O_PTA_REMALLOC_SERVICE_PT_TID                  0x0002
#define    I2O_PTA_REMALLOC_SERVICE_REM_ALLOC_SIZE          0x0003
#define    I2O_PTA_REMALLOC_SERVICE_REMOTE_UNIT             0x0004
#define    I2O_PTA_REMALLOC_SERVICE_REMOTE_IOP              0x0005

typedef struct _I2O_PTA_REMALLOC_SERVICE_TABLE {
    U32    PsuIndex;
    U16    PsuTid;
    U16    PtTid;
    U32    RemMemSize;
    U16    RemUnit;
    U16    RemIop;
} I2O_PTA_REMALLOC_SERVICE_TABLE, *PI2O_PTA_REMALLOC_SERVICE_TABLE;

/* - 0007h - PSU Receive Service Group */

#define    I2O_PTA_RECEIVE_SERVICE_PSU_INDEX                0x0000
#define    I2O_PTA_RECEIVE_SERVICE_PSU_TID                  0x0001

typedef struct _I2O_PTA_RECEIVE_SERVICE_TABLE {
    U32    PsuIndex;
    U16    PsuTid;
} I2O_PTA_RECEIVE_SERVICE_TABLE, *PI2O_PTA_RECEIVE_SERVICE_TABLE;


/************************************************************/
/* Peer Transport Group 0000h - Information Parameter Group */
/************************************************************/

#define PTRANSPORT_INFO_GROUP           0x0000

/* Field Indices for Information Parameter Group */

#define PT_INFO_PT_MEDIA_TYPE           0
#define PT_INFO_PT_ID                   1
#define PT_INFO_PT_SEGMENT_TYPE         2
#define PT_INFO_PT_PRIORITY             3
#define PT_INFO_RESERVED1               4
#define PT_INFO_PT_QOS                  5
#define PT_INFO_PT_DATA_SIZE_REQUESTED  6
#define PT_INFO_PT_DATA_SIZE            7
#define PT_INFO_PT_ELEMENT_SIZE         8
#define PT_INFO_PT_SERVICE_FLAGS        9
#define PT_INFO_CONNECTED_IOPS          10
#define PT_INFO_PT_CONNECT_INFO_SIZE    11
#define PT_INFO_PT_CONNECT_INFO         12

/* PT Media Types - 4 bytes */

#define PT_INFO_MEDIA_TYPE_NULL         0x00000000
#define PT_INFO_MEDIA_TYPE_PCI          0x00000001
#define PT_INFO_MEDIA_TYPE_OTHER        0xffffffff

/* PT IDs - 4 bytes */

#define PT_INFO_PT_ID_UNASSIGNED        0xffffffff

/* PT Segment Types - 1 byte */

#define PT_INFO_SEGMENT_TYPE_SHARED     0x00
#define PT_INFO_SEGMENT_TYPE_SEGMENTED  0x01

/* PT Priorities - 1 byte */

#define PT_INFO_PRIORITY_DEFAULT         0

/* PT Quality Of Service - 4 bytes - See i2oPeerLib.h */

/* PT Service Flags - 4 bytes */

#define PT_INFO_FLAGS_DATA_LINK_REDUNDANCY  1 << 0

typedef struct _PTRANSPORT_INFO_SCALAR {
    U32           PtMediaType;
    U32           PtId;
    U8            PtSegmentType;
    U8            PtPriority;
    U8            Reserved[2];
    U32           PtQos;
    U32           PtDataSizeRequested;
    U32           PtDataSize;
    U32           PtElementSize;
    U32           PtServiceFlags;
    U32           ConnectedIOPs;
    U32           PtConnectInfoSize;
    U32           PtConnectInfo;
} PTRANSPORT_INFO_SCALAR, *PPTRANSPORT_INFO_SCALAR;

/**************************************************/
/* Peer Transport Group 0001h - IOP Connect Table */
/**************************************************/

#define PTRANSPORT_IOP_CONNECT_GROUP    0x0001

/* Field Indices for IOP Connect Table */

#define PT_IOP_CONNECT_UNIT             1
#define PT_IOP_CONNECT_IOP              2

typedef struct _PTRANSPORT_IOP_CONNECT_TABLE {
    U32           ConnectIopIndex;
    U16           ConnectUnit;
    U16           ConnectIOP;
} PTRANSPORT_IOP_CONNECT_TABLE, *PPTRANSPORT_IOP_CONNECT_TABLE;

/*******************************************************************/
/* Peer Transport Group 0002h - Remote IOP Memory Allocation Group */
/*******************************************************************/

#define PTRANSPORT_REM_IOP_ALLOC_GROUP  0x0002

/* Field Indices for Remote IOP Memory Allocation Group */

#define PT_REM_IOP_ALLOC_NUM_REMOTE_IOP             0
#define PT_REM_IOP_ALLOC_TOTAL_ALLOCATED_MEM        1

typedef struct _PTRANSPORT_REM_IOP_ALLOC_SCALAR {
    U32           NumRemoteIop;
    U64           TotalAllocatedMem;
} PTRANSPORT_REM_IOP_ALLOC_SCALAR, *PPTRANSPORT_REM_IOP_ALLOC_SCALAR;

/*******************************************************************/
/* Peer Transport Group 0003h - IOP Remote Memory Allocation Table */
/*******************************************************************/

#define PTRANSPORT_REM_MEM_ALLOC_GROUP  0x0003

/* Field Indices for IOP Remote Memory Allocation Table */

#define PT_REM_MEM_ALLOC_REM_UNIT                   1
#define PT_REM_MEM_ALLOC_REM_IOP                    2
#define PT_REM_MEM_ALLOC_RESERVED                   3
#define PT_REM_MEM_ALLOC_TOTAL_REDIRECT_SIZE        4
#define PT_REM_MEM_ALLOC_TOTAL_REM_MEM_SIZE         5

typedef struct _PTRANSPORT_REM_MEM_ALLOC_TABLE {
    U16           IopIndex;
    U16           RemUnit;
    U16           RemIop;
    U16           Reserved;
    U32           TotalRedirecSize;
    U32           TotalRemMemSize;
} PTRANSPORT_REM_MEM_ALLOC_TABLE, *PPTRANSPORT_REM_MEM_ALLOC_TABLE;

#endif    /* I2O_PEER_HDR */
