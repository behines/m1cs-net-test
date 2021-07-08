/**
 *****************************************************************************
 *
 * @file GlcMsg.h
 *	GLC Message Declarations.
 *
 *	This header file defines general message structures and symbolic
 *	constants for message IDs that are common to all GLC components.
 *	Message structure definitions for specific interfaces are in separate
 *	header files for each interface.
 *
 * @par Project
 *	TMT Primary Mirror Control System (M1CS) \n
 *	Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author	Thang Trinh
 * @date	28-Jun-2021 -- Initial delivery.
 *
 * Copyright (c) 2015-2021, California Institute of Technology
 *
 ****************************************************************************/

#ifndef GLCMSG_H_
#define GLCMSG_H_

#include "Glc.h"

/// system wide message ID's
#define CMD_TYPE        (1<<12)
#define BIN_CMD_TYPE    (CMD_TYPE +1)   //!< Binary command for testing.
#define RSP_TYPE        (2<<12)
#define ALARM_TYPE      (3<<12)
#define DATA_TYPE       (5<<12)

#define OS_PACK  __attribute__ ((packed, aligned(2))) // Force alignment for data structures 

/// system wide data type ID's
typedef enum data_id {
    XXX_STATUS_DATA = DATA_TYPE + 1,	//!< XXX status data

    XXX_RESERVED_1,			//!< Reserved
    XXX_RESERVED_2,			//!< Reserved

    MAX_DATA_ID				//!< Maximum valid message id
} DATA_ID;

/// message header definition -- Note header's are in network byte order.
struct MsgHdr {
    uint16_t msgId;              //!< message type 
    uint16_t srcId;              //!< sender application id 
    uint16_t msgLen;             //!< message length including header(bytes) 
    uint16_t seqNo;              //!< sequence number 
} OS_PACK;

#define MAX_CMD_LEN     (256)
#define MAX_RSP_LEN     (256)
#define MAX_ALARM_LEN   (256)

struct CmdMsg {
    MsgHdr hdr;
    char   cmd[MAX_CMD_LEN];
} OS_PACK;

struct RspMsg {
    MsgHdr hdr;
    char   rsp[MAX_RSP_LEN];
} OS_PACK;

struct TimeTag {
    uint32_t tv_sec;
    uint32_t tv_nsec;
} OS_PACK;

struct DataHdr {
    MsgHdr  hdr;
    TimeTag time;
} OS_PACK;

/// Alarm severity levels
enum AlarmLevel {
    LVL_INFO,                    //!< For informational or diagnostic purposes
    LVL_WARN,                    //!< For off-nominal conditions that are non-critical
    LVL_ERROR,                   //!< For error conditions that are recoverable
    LVL_CRITICAL                 //!< For critical non-recoverable error conditions
};

/// Message structure to report alarms into system log.
struct AlarmMsg {
    DataHdr    hdr;
    AlarmLevel level;
    char       message[MAX_ALARM_LEN];
} OS_PACK;

/// 
/// Message to allow sending raw block of data directly to the internal 
/// devices test interface. 
///
struct TestCmdMsg {
    MsgHdr   hdr;
    uint16_t dest;                //!< Destination device 1-3 USEB, 4-6 Actuator 
    uint8_t  *payload;
} OS_PACK;

#endif /* GLCMSG_H_ */

