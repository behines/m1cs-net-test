// net.h -- Network Communication Services Type Declarations

/*----------------------------------------------------------------------------
 * Copyright (c) 1995-2010,2015, Jet Propulsion Laboratory
 * Permission is granted to make and distribute copies of this software
 * without fee, provided the above copyright notice and this permission notice
 * are preserved on all copies.  All other rights reserved.  The software is
 * provided "as is" without express or implied warranty, and no representation
 * is made about its suitability for any purpose.
 *
 * Revision History:
 *
 *   Date            By         Description
 *
 * 13-Oct-95       T. Trinh     Initial release (v1.0).
 * 08-Apr-96       T. Trinh     Add VxWorks broadcast support.
 * 14-Sep-15       T. Trinh     Change NET_MAX_FD from 128 to 1024 (under Linux,
 *                              limits can be changed via /etc/security/limits.conf).
 *
 * Description:
 *    This header file contains type declarations and symbolic
 *    constant definitions that are used internally by the network
 *    communication services.
 *
 *--------------------------------------------------------------------------*/

#ifndef NET_H
#define NET_H

#include "acs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NET_MAX_ENDPTS       (23) //!< max number of remote endpoints
#define NET_MAX_FD         (1024) //!< max number of open socket desc

#define NET_MIN_MSG_LEN  (sizeof (char)) //!< minimum message length
#define NET_MAX_MSG_LEN      (4097*1024) //!< maximum message length (> 4Mb)

#define NET_MAX_UDP_LEN     (4096) //!< maximum UDP packet length

#define NET_MIN_USEC_DELAY (20000) //!< minimum delay in microseconds
#define NET_MAX_NDELAY        (10) //!< max number of delays before
                                   //!< returning NWOULDBLOCK

typedef enum {
    UNDEF, TCP, UDP, BRDCST
} endpt_type;

/// listener's endpoint entry

typedef struct endpt_entry {
    char       *name;       //!< endpoint (or service) name
    endpt_type type;        //!< endpoint protocol
    int        pname;       //!< server's name
    int        port;        //!< endpoint port number
} endpt_entry;

/// open socket descriptor entry

typedef struct sockfd_entry {
    endpt_type type;        //!< socket type
    io_mode    mode;        //!< socket I/O mode
} sockfd_entry;
 
extern endpt_entry net_endpt[];  //!< list of endpoint entries
extern int           net_port[]; //!< list of port numbers bound to
                                 //!< by a client

#ifdef __cplusplus
} // extern "C"
#endif

#endif // NET_H
