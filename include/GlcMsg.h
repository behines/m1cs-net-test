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

#include "net.h"
#include <stdint.h>
#include <time.h>

/// system wide message ID's
#define CMD_TYPE             (1 << 16)
#define RSP_TYPE             (2 << 16)
#define LOG_TYPE             (3 << 16)
#define DATA_TYPE            (4 << 16)

#define OS_PACK __attribute__((packed, aligned(2))) // Force alignment for data structures

/// system wide data type ID's
typedef enum data_id {
    SEG_STATUS_DATA = DATA_TYPE + 1, //!< SegmentStatusMsg
    RAW_DATA,                        //!< RawDataMsg data
    WH_STRAIN_DATA,                  //!< WarpHarnStrainMsg
    WH_CALIB_DATA,                   //!< WarpHarnCalibMsg
    SENS_CFG_DATA,                   //!< SensConfigMsg
    ACT_CFG_DATA,                    //!< ActConfigMsg
    SEG_REALTIME_DATA,               //!< SegRtDataMsg
    ACT_REALTIME_DATA,               //!< ActTargetMsg
    XXX_RESERVED_1,                  //!< Reserved
    XXX_RESERVED_2,                  //!< Reserved
    MAX_DATA_ID //!< Maximum valid message id
} DATA_ID;

/// message header definition
typedef struct MsgHdr {
    uint32_t msgId;  //!< message type
    uint32_t srcId;  //!< sender application id
//    uint32_t msgLen; //!< message length including header(bytes)
//    uint32_t seqNo;  //!< sequence number
} OS_PACK MsgHdr;

#define MAX_CMD_LEN          (256)
#define MAX_RSP_LEN          (256)
#define MAX_LOG_LEN          (256)

typedef struct CmdMsg {
    MsgHdr hdr;
    char cmd[MAX_CMD_LEN];
} OS_PACK CmdMsg;

typedef struct RspMsg {
    MsgHdr hdr;
    char rsp[MAX_RSP_LEN];
} OS_PACK RspMsg;

#if 0
typedef struct TimeTag {
    uint64_t tv_sec;
    uint64_t tv_nsec;
} OS_PACK TimeTag;
#endif

#include <sys/time.h>

typedef struct timeval TimeTag;

typedef struct DataHdr {
    MsgHdr hdr;
    TimeTag time;
} OS_PACK DataHdr;

/// Log severity levels
typedef enum LogLevel {
    LOG_FATAL,    //!< For errors that cause the system to halt
    LOG_CRITICAL, //!< For critical non-recoverable error conditions
    LOG_ERROR,    //!< For error conditions that are recoverable
    LOG_WARN,     //!< For off-nominal conditions that are non-critical
    LOG_INFO,     //!< For informational or diagnostic purposes
    LOG_DEBUG,    //!< For debug purposes
    LOG_TRACE,    //!< For system trace logging
    NUM_LOG_LEVELS
} LogLevel;

/// Message structure to report alarms into system log.
typedef struct LogMsg {
    DataHdr hdr;
    LogLevel level;
    char message[MAX_LOG_LEN];
} OS_PACK LogMsg;

///
/// Message to allow sending raw block of data directly to the internal
/// devices test interface.
///
typedef struct RawDataMsg {
    MsgHdr   hdr;
    uint16_t dest;    //!< Destination device 1-3 USEB, 4-6 Actuator
    uint16_t dataLen; //!< number of bytes of raw data
    uint8_t  rawData[NET_MAX_MSG_LEN - 2 * sizeof(MsgHdr)];
} OS_PACK RawDataMsg;

#endif /* GLCMSG_H_ */
