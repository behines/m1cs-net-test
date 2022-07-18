/* acs.h -- Application Common Services(ACS) Global Type Declarations */

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
 * 
 * Description:
 *  This header file defines global type definitions and symbolic
 *  constants that are common to all application common services.
 *
 *--------------------------------------------------------------------------*/

#ifndef ACS_H
#define ACS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SUCCESS
#define SUCCESS (0)
#endif

#ifndef ERROR 
#define ERROR   (-1) 
#endif

#ifndef NULL
#define NULL    (0)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (1)
#endif

#define ALREADY_INITIALIZED (-2)
#define NOT_INITIALIZED     (-3)

typedef enum {
    BLOCKING = 0,           /* return after I/O completes */
    NON_BLOCKING = 1        /* return immediately, I/O done or not */
} io_mode;

#ifdef __cplusplus
} /* extern "C" */
#endif
 
#endif /* #ifndef ACS_H */
