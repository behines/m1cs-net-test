/* net_appl.h -- Network Communication Application Type Declarations */

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
 *   Date            By               Description
 *
 * 12-Oct-95     Thang Trinh        Initial release (v1.0).
 * 08-Apr-96     Thang Trinh        Add VxWorks broadcast support.
 * 20-Nov-96     Thang Trinh        Version 2.0 release to support generic
 *                                  application names.
 *
 * Description:
 *    This header file contains common type declarations and symbolic
 *    constant definitions that are required by applications using the 
 *    Network Communication Services.
 *
 *--------------------------------------------------------------------------*/

#ifndef NET_APPL_H
#define NET_APPL_H

#include <unistd.h>

#include "acs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// generic application names

#define ANY_TASK       (0)        //!< wildcard task
#define SGW_TASK       (1)        //!< standard gateway/comm task
#define CTL_TASK       (2)        //!< standard control task
#define MON_TASK       (3)        //!< standard monitor task
#define APP1_TASK      (4)        //!< generic tasks
#define APP2_TASK      (5)
#define APP3_TASK      (6)
#define APP4_TASK      (7)
#define APP5_TASK      (8)
#define APP6_TASK      (9)
#define APP7_TASK     (10)
#define APP8_TASK     (11)
#define APP9_TASK     (12)
#define APP10_TASK    (13)
#define APP11_TASK    (14)
#define APP12_TASK    (15)
#define APP13_TASK    (16)
#define APP14_TASK    (17)
#define APP15_TASK    (18)
#define APP16_TASK    (19)
#define APP17_TASK    (20)
#define APP18_TASK    (21)
#define APP19_TASK    (22)
#define APP20_TASK    (23)

#define MAXTASKS    (APP20_TASK+1)    //!< max number of client tasks

/// generic endpoint names 

#define SGW_SRV        ("sgw_srv")        //!< standard gateway server
#define CTL_SRV        ("ctl_srv")        //!< standard control server
#define MON_SRV        ("mon_srv")        //!< standard monitor server

#define APP_SRV1      ("app_srv1")        //!< generic servers
#define APP_SRV2      ("app_srv2")
#define APP_SRV3      ("app_srv3")
#define APP_SRV4      ("app_srv4")
#define APP_SRV5      ("app_srv5")
#define APP_SRV6      ("app_srv6")
#define APP_SRV7      ("app_srv7")
#define APP_SRV8      ("app_srv8")
#define APP_SRV9      ("app_srv9")
#define APP_SRV10    ("app_srv10")
#define APP_SRV11    ("app_srv11")
#define APP_SRV12    ("app_srv12")
#define APP_SRV13    ("app_srv13")
#define APP_SRV14    ("app_srv14")
#define APP_SRV15    ("app_srv15")
#define APP_SRV16    ("app_srv16")
#define APP_SRV17    ("app_srv17")
#define APP_SRV18    ("app_srv18")
#define APP_SRV19    ("app_srv19")
#define APP_SRV20    ("app_srv20")

#define ANT_BRDCST   ("ant_brdcst")     //!< broadcast endpoint
#define ANT_NETWRK   ("ei0")            //!< broadcast network

/// generic server names

#define SRV1_TASK     (101)
#define SRV2_TASK     (102)
#define SRV11_TASK    (111)
#define SRV12_TASK    (112)
#define SRV13_TASK    (113)
#define SRV14_TASK    (114)
#define SRV15_TASK    (115)
#define SRV16_TASK    (116)
#define SRV17_TASK    (117)
#define SRV18_TASK    (118)
#define SRV19_TASK    (119)
#define SRV20_TASK    (120)

/// function prototypes

int net_init (char *endpt);
int net_accept (int listenfd, io_mode mode);
int net_connect (char *endpt, char *hostname, int pname, io_mode mode);
int net_send (int sockfd, char *msg, int length, io_mode mode);
int net_recv (int sockfd, char *buf, int maxlen, io_mode mode);
int net_getpeername (int sockfd, int *pname, char *hostname, int namelen);
int net_setiomode (int sockfd, io_mode mode);
int net_close (int sockfd);

/// function return values

#define NEOF         (0)
#define NBADADDR    (-2)
#define NBADENDPT   (-3)
#define NBADFD      (-4)
#define NBADHOST    (-5)
#define NBADLENGTH  (-6)
#define NBADMODE    (-7)
#define NBADPROCESS (-8)
#define NSYNCERR    (-9)
#define NWOULDBLOCK (-10)
 
/// macro for formatting net services error string

#ifndef NET_ERRSTR
#define NET_ERRSTR(errcode) \
    ((errcode == NEOF) ? "End of file detected" : \
    ((errcode == NBADADDR) ? "Invalid pointer" : \
    ((errcode == NBADENDPT) ? "Invalid endpoint name" : \
    ((errcode == NBADFD) ? "Invalid socket descriptor" : \
    ((errcode == NBADHOST) ? "Invalid host name" : \
    ((errcode == NBADLENGTH) ? "Invalid message length" : \
    ((errcode == NBADMODE) ? "Invalid I/O mode" : \
    ((errcode == NBADPROCESS) ? "Invalid process name" : \
    ((errcode == NSYNCERR) ? "Out of sync message" : \
    ((errcode == NWOULDBLOCK) ? "Operation would block" : \
    ((errcode == ERROR) ? "System call error" : \
    "<illegal value>")))))))))))
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // #ifndef NET_APPL_H

