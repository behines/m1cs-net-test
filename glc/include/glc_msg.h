/**
 *****************************************************************************
 *
 * @file glc_msg.h
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

#ifndef GLC_MSG_H
#define GLC_MSG_H

#include "glc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* system wide message ID's */

#define CMD_TYPE	(1<<12)
#define RSP_TYPE	(2<<12)
#define EVENT_TYPE	(3<<12)
#define DATA_TYPE	(4<<12)

/* system wide data type ID's */

typedef enum data_id {
    XXX_STATUS_DATA = DATA_TYPE + 1,	//!< XXX status data

    XXX_RESERVED_1,			//!< Reserved
    XXX_RESERVED_2,			//!< Reserved

    MAX_DATA_ID				//!< Maximum valid message id
} DATA_ID;

/* message header definition */

typedef struct msg_hdr {
    U32  msg_id;		//!< message id
    U32  src_id;		//!< message sender id
} MSG_HDR;

#define MAX_CMD_LEN		256
#define MAX_RSP_LEN		256
#define MAX_EVENT_LEN		256

typedef struct cmd_msg {
    MSG_HDR hdr;
    char    cmd[MAX_CMD_LEN];
} CMD_MSG;

typedef struct rsp_msg {
    MSG_HDR hdr;
    char    rsp[MAX_RSP_LEN];
} RSP_MSG;

/*  TIME_TAG definition */

#include <sys/time.h>

typedef struct timeval TIME_TAG;

typedef enum {
	LVL_INFO,		//!< For informational or diagnostic purposes
	LVL_WARN,		//!< For off-nominal conditions that are non-critical
	LVL_ERROR,		//!< For error conditions that are recoverable
	LVL_CRITICAL		//!< For critical non-recoverable error conditions
} EVENT_LVL;

typedef struct event_msg {
    MSG_HDR   hdr;
    TIME_TAG  time;
    EVENT_LVL level;		//!< event severity level
    char      event[MAX_EVENT_LEN];
} EVENT_MSG;

typedef struct data_hdr {
    MSG_HDR  hdr;
    TIME_TAG time;
} DATA_HDR;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GLC_MSG_H */

