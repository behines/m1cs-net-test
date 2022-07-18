/* net_endpt.c -- Network Endpoint Names */

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
 * 13-Oct-95     Thang Trinh        Initial Release (v1.0).
 * 08-Apr-96     Thang Trinh        Add VxWorks broadcast endpoint.
 *
 * Description:
 *	This module defines and initializes the list of listening or servers'
 *	endpoint names, and the array of port numbers that are bound to by
 *	a client.
 *
 *--------------------------------------------------------------------------*/

#include "net_appl.h"
#include "net.h"

/* servers' endpoint names */

endpt_entry net_endpt[] = {

/*  endpoint     type    server       port */

    {SGW_SRV,    TCP,    SGW_TASK,    8001},
    {CTL_SRV,    TCP,    CTL_TASK,    8002},
    {MON_SRV,    TCP,    MON_TASK,    8003},
    {APP_SRV1,   TCP,    SRV1_TASK,   8004},
    {APP_SRV2,   TCP,    SRV2_TASK,   8005},
    {APP_SRV3,   TCP,    APP1_TASK,   8006},
    {APP_SRV4,   TCP,    APP2_TASK,   8007},
    {APP_SRV5,   TCP,    APP3_TASK,   8008},
    {APP_SRV6,   TCP,    APP4_TASK,   8009},
    {APP_SRV7,   TCP,    APP5_TASK,   8010},
    {APP_SRV8,   TCP,    APP6_TASK,   8011},
    {APP_SRV9,   TCP,    APP7_TASK,   8012},
    {APP_SRV10,  TCP,    APP8_TASK,   8013},
    {APP_SRV11,  TCP,    SRV11_TASK,  8014},
    {APP_SRV12,  TCP,    SRV12_TASK,  8015},
    {APP_SRV13,  TCP,    SRV13_TASK,  8016},
    {APP_SRV14,  TCP,    SRV14_TASK,  8017},
    {APP_SRV15,  TCP,    SRV15_TASK,  8018},
    {APP_SRV16,  TCP,    SRV16_TASK,  8019},
    {APP_SRV17,  TCP,    SRV17_TASK,  8020},
    {APP_SRV18,  TCP,    SRV18_TASK,  8021},
    {APP_SRV19,  TCP,    SRV19_TASK,  8022},
    {APP_SRV20,  TCP,    SRV20_TASK,  8023},

    {ANT_BRDCST, BRDCST, 0,	      8101}
};

/* port numbers bound to by a client, so that a listener can
   identify the connector */

int net_port[] = {

       0, 9001, 9002, 9003, 9004, 9005, 9006, 9007, 9008, 9009,
    9010, 9011, 9012, 9013, 9014, 9015, 9016, 9017, 9018, 9019,
    9020, 9021, 9022, 9023
};
